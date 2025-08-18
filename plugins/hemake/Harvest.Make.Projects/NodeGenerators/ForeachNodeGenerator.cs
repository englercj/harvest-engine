// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Nodes;
using System.Text;

namespace Harvest.Make.Projects.NodeGenerators;

public class ForeachNodeGeneratorTraits : NodeGeneratorBaseTraits
{
    public override string Name => "foreach";
}

public class ForeachNodeGenerator(IProjectService projectService) : NodeGeneratorBase<ForeachNodeGeneratorTraits>(projectService)
{
    public override void GenerateNodes(KdlNode generatorNode, KdlNode scope)
    {
        if (!generatorNode.TryGetArgumentValue(0, out string? nodeType) || string.IsNullOrEmpty(nodeType))
        {
            throw new NodeParseException(generatorNode, "Expected string argument in foreach generator node for the module name or plugin ID.");
        }

        if (nodeType == PluginNode.NodeTraits.Name)
        {
            foreach (PluginNode plugin in _projectService.GetAllPlugins())
            {
                if (DoesNodeMatch(generatorNode, plugin))
                {
                    GenerateNodes(generatorNode, scope, $"plugin[{plugin.PluginName}]");
                }
            }
        }
        else if (nodeType == ModuleNode.NodeTraits.Name)
        {
            foreach (ModuleNode module in _projectService.GetAllModules())
            {
                if (DoesNodeMatch(generatorNode, module))
                {
                    GenerateNodes(generatorNode, scope, $"module[{module.ModuleName}]");
                }
            }
        }
        else
        {
            throw new NodeParseException(generatorNode, $"Unknown foreach generator node type '{nodeType}'. Expected '{PluginNode.NodeTraits.Name}' or '{ModuleNode.NodeTraits.Name}'.");
        }
    }

    private static void GenerateNodes(KdlNode generatorNode, KdlNode scope, string contextName)
    {
        TokenHandler handler = new(contextName);
        StringTokenReplacer replacer = new(handler);

        foreach (KdlNode source in generatorNode.Children)
        {
            KdlNode generated = GenerateNode(replacer, source);
            scope.AddChild(generated);
        }
    }

    private static KdlNode GenerateNode(StringTokenReplacer replacer, KdlNode source)
    {
        string resolvedName = replacer.ReplaceTokens(source.Name);
        KdlNode result = new(resolvedName, source.Type) { SourceInfo = source.SourceInfo };

        // Copy properties, resolving any string values using the replacer
        foreach (KdlValue value in source.Arguments)
        {
            if (value is KdlString valueStr)
            {
                string resolvedValue = replacer.ReplaceTokens(valueStr.Value);
                result.Arguments.Add(new KdlString(resolvedValue, valueStr.Type) { SourceInfo = value.SourceInfo });
            }
            else
            {
                // For non-string values, just clone them directly
                result.Arguments.Add(value.Clone());
            }
        }

        // Copy properties, resolving any string values using the replacer
        foreach ((string key, KdlValue value) in source.Properties)
        {
            if (value is KdlString valueStr)
            {
                string resolvedValue = replacer.ReplaceTokens(valueStr.Value);
                result.Properties[key] = new KdlString(resolvedValue, valueStr.Type) { SourceInfo = value.SourceInfo };
            }
            else
            {
                result.Properties[key] = value.Clone();
            }
        }

        // Recursively generate child nodes
        foreach (KdlNode child in source.Children)
        {
            result.AddChild(GenerateNode(replacer, child));
        }

        return result;
    }

    private static bool DoesNodeMatch<T>(KdlNode generatorNode, T candidate) where T : INode
    {
        foreach ((string key, KdlValue value) in generatorNode.Properties)
        {
            // Special case for arguments
            if (key.StartsWith("_arg"))
            {
                if (!int.TryParse(key[4..], out int argIndex))
                {
                    throw new Exception($"Invalid foreach filter '{key}'. Argument index must be an integer.");
                }

                if (argIndex > 0 && argIndex < candidate.Node.Arguments.Count)
                {
                    return DoesValueMatch(value, candidate.Node.Arguments[argIndex]);
                }

                // If there is no argument at that index, it doesn't match
                return false;
            }

            if (!candidate.Node.Properties.TryGetValue(key, out KdlValue? candidateValue))
            {
                return false;
            }

            return DoesValueMatch(value, candidateValue);

        }
        return true;
    }

    private static bool DoesValueMatch(KdlValue filterValue, KdlValue candidateValue)
    {
        // If the filter value is a string we need to evaluate it as an expression
        if (filterValue is KdlString expr)
        {
            return WhenExpressionEvaluator.Evaluate(expr.Value, candidateValue);
        }

        return filterValue.Equals(candidateValue);
    }

    private class TokenHandler(string resolvedContextName) : IStringTokenHandler
    {
        public string GetTokenValue(StringTokenContext tokenContext)
        {
            if (tokenContext.ContextName == "_entry")
            {
                if (!string.IsNullOrEmpty(tokenContext.ContextId))
                {
                    throw new Exception($"Invalid token '{tokenContext.Token}'. The '_entry' context cannot be indexed.");
                }

                StringBuilder sb = new();
                sb.Append(tokenContext.Token[..tokenContext.ContextNameCapture.Index]);
                sb.Append(resolvedContextName);
                sb.Append(tokenContext.Token[(tokenContext.ContextNameCapture.Index + tokenContext.ContextNameCapture.Length)..]);

                return sb.ToString();
            }

            return tokenContext.Token;
        }
    }
}
