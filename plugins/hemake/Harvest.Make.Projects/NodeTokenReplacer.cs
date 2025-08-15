// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Text.RegularExpressions;

namespace Harvest.Make.Projects;

/// <summary>
/// Function for resolving a parsed token to a string value.
/// </summary>
/// <param name="projectContext">The project context.</param>
/// <param name="token">The full token string, including transformers.</param>
/// <param name="contextName">The parsed context name from the token.</param>
/// <param name="propertyName">The parsed property name from the token.</param>
/// <param name="value">The resolved value, if the resolver was able to resolve it.</param>
/// <returns>True if the value was resolved or false if no special handling was applied.</returns>
public delegate bool NodeTokenResolver(ProjectContext projectContext, string contextName, string propertyName, [MaybeNullWhen(false)] out string value);

/// <summary>
/// Function for transforming a token value.
/// </summary>
/// <param name="input">The input string to transform.</param>
/// <returns>The transformed string</returns>
public delegate string NodeTokenTransformer(string input);

public partial class NodeTokenReplacer(
    IReadOnlyDictionary<string, NodeTokenResolver> resolversByContextName,
    IReadOnlyDictionary<string, NodeTokenTransformer>? transformersByName = null)
{
    public static readonly IReadOnlyDictionary<string, NodeTokenTransformer> DefaultTransformers = new Dictionary<string, NodeTokenTransformer>()
    {
        { "lower", (v) => v.ToLower() },
        { "upper", (v) => v.ToUpper() },
        { "trim", (v) => v.Trim() },
        { "dirname", (v) => Path.GetDirectoryName(v) ?? "" },
        { "basename", (v) => Path.GetFileName(v) ?? "" },
        { "extname", (v) => Path.GetExtension(v)?.TrimStart('.') ?? "" },
        { "extension", (v) => Path.GetExtension(v) ?? "" },
        { "noextension", (v) => Path.ChangeExtension(v, null) ?? "" },
    };

    private const string ArgumentKeyPrefix = "_arg";
    private const string TokenRegexPattern = @"\$\{[^\}]+\}";
    private const string TokenNameIdRegexPattern = @"^(module|plugin)\[([^\]]+)\]$";

    [GeneratedRegex(TokenRegexPattern, RegexOptions.Singleline)]
    private static partial Regex TokenRegex();

    [GeneratedRegex(TokenNameIdRegexPattern, RegexOptions.Singleline)]
    private static partial Regex TokenNameIdRegex();

    public IReadOnlyDictionary<string, NodeTokenResolver> Resolvers => resolversByContextName;
    public IReadOnlyDictionary<string, NodeTokenTransformer> Transformers => transformersByName ?? DefaultTransformers;

    public string ReplaceTokens(ProjectContext projectContext, KdlNode scope, string input)
    {
        return TokenRegex().Replace(input, (match) =>
        {
            string token = match.Value[2..^1]; // ${token} -> token
            string[] contextParts = token.Split('.');

            if (contextParts.Length != 2)
            {
                throw new NodeParseException(scope, $"Invalid token '{token}'. Expected format: 'context.property'.");
            }

            string[] transformerParts = contextParts[1].Split(':');

            string contextName = contextParts[0];
            string propertyName = transformerParts[0];
            string tokenValue = GetTokenValue(projectContext, scope, token, contextName, propertyName);

            foreach (string transformerName in transformerParts[1..])
            {
                if (Transformers.TryGetValue(transformerName, out NodeTokenTransformer? transformer))
                {
                    tokenValue = transformer(tokenValue);
                }
                else
                {
                    throw new NodeParseException(scope, $"Invalid token '{token}'. Unknown transformer '{transformer}'.");
                }
            }

            return tokenValue;
        });
    }

    private string GetTokenValue(ProjectContext projectContext, KdlNode scope, string token, string contextName, string propertyName)
    {
        // If a resolver is registered for this context name, then try to use that directly.
        if (Resolvers.TryGetValue(contextName, out NodeTokenResolver? resolver))
        {
            if (resolver(projectContext, contextName, propertyName, out string? resolvedValue))
            {
                return resolvedValue;
            }
        }

        // If no resolver is registered for this context name, then try to do some generic lookups.
        KdlNode nodeContext = GetScopeWithName(projectContext, scope, token, contextName);

        if (IsArgumentKey(propertyName))
        {
            return GetArgumentTokenValue(nodeContext, token, contextName, propertyName);
        }

        if (nodeContext.Properties.TryGetValue(propertyName, out KdlValue? value))
        {
            return value.GetValueString();
        }

        throw new NodeParseException(scope, $"Invalid token '{token}'. Unknown property '{propertyName}' on context '{contextName}'.");
    }

    private static bool IsArgumentKey(string propertyName)
    {
        return propertyName.StartsWith(ArgumentKeyPrefix);
    }

    private static int GetArgumentIndexFromKey(KdlNode node, string propertyName)
    {
        Debug.Assert(IsArgumentKey(propertyName));

        if (!int.TryParse(propertyName[ArgumentKeyPrefix.Length..], out int argIndex))
        {
            throw new NodeParseException(node, $"Invalid argument key '{propertyName}'. Argument index must be an integer.");
        }

        return argIndex;
    }

    private static string GetArgumentTokenValue(KdlNode node, string token, string contextName, string propertyName)
    {
        int argIndex = GetArgumentIndexFromKey(node, propertyName);

        if (argIndex < 0 && argIndex >= node.Arguments.Count)
        {
            throw new NodeParseException(node, $"Invalid token '{token}'. Argument index {argIndex} is out of range for context '{contextName}'.");
        }

        return node.Arguments[argIndex].GetValueString();
    }

    private static KdlNode GetScopeWithName(ProjectContext projectContext, KdlNode scope, string token, string name)
    {
        Match match = TokenNameIdRegex().Match(name);
        if (match.Success && match.Groups.Count == 3)
        {
            string contextType = match.Groups[1].Value;
            string contextId = match.Groups[2].Value;

            if (contextType == PluginNode.NodeTraits.Name)
            {
                return projectContext.ProjectService.TryGetPluginById(contextId)?.Node
                    ?? throw new NodeParseException(scope, $"Invalid token '{token}'. No plugin found with name '{contextId}'.");
            }
            else if (contextType == ModuleNode.NodeTraits.Name)
            {
                return projectContext.ProjectService.TryGetModuleByName(contextId)?.Node
                    ?? throw new NodeParseException(scope, $"Invalid token '{token}'. No module found with name '{contextId}'.");
            }
            else
            {
                throw new NodeParseException(scope, $"Invalid token '{token}'. Context type '{contextType}' cannot be indexed.");
            }
        }

        KdlNode? search = scope;
        while (search is not null)
        {
            if (search.Name == name)
            {
                return search;
            }

            search = search.Parent;
        }

        throw new NodeParseException(scope, $"Invalid token '{token}'. Failed to find a '{name}' node in the parent heirarchy.");
    }
}
