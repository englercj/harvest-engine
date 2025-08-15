// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Nodes;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Make.Projects.NodeGenerators;

public class ForeachNodeGeneratorTraits : NodeGeneratorBaseTraits
{
    public override string Name => "foreach";
}

public class ForeachNodeGenerator(ProjectContext context) : NodeGeneratorBase<ForeachNodeGeneratorTraits>(context)
{
    private class ForeachReplacerContext : ProjectContext
    {
        private readonly NodeTokenReplacer _replacer = new(new Dictionary<string, NodeTokenResolver>()
        {
            { "_entry", EntryTokenResolver },
        });

        public INode ContextNode { get; }

        public ForeachReplacerContext(ProjectContext parentContext, INode contextNode)
            : base(parentContext.ProjectService)
        {
            Plugin = parentContext.Plugin;
            Module = parentContext.Module;
            Configuration = parentContext.Configuration;
            Platform = parentContext.Platform;
            Host = parentContext.Host;
            Options = parentContext.Options;
            Tags = parentContext.Tags;

            ContextNode = contextNode;
        }

        public string ReplaceTokens(KdlNode source, string value)
        {
            // Use the replacer to resolve tokens in the value string
            return _replacer.ReplaceTokens(this, source, value);
        }

        private static bool EntryTokenResolver(ProjectContext projectContext, string contextName, string propertyName, [MaybeNullWhen(false)] out string value)
        {
            Debug.Assert(contextName == "_entry");
            Debug.Assert(projectContext is ForeachReplacerContext);

            ForeachReplacerContext foreachContext = (ForeachReplacerContext)projectContext;
            string targetContextName = foreachContext.ContextNode.Node.Name;

            // If a resolver is registered for this context name, then try to use that directly.
            if (projectContext.ProjectService.TokenReplacer.Resolvers.TryGetValue(targetContextName, out NodeTokenResolver? resolver))
            {
                if (resolver(projectContext, targetContextName, propertyName, out string? resolvedValue))
                {
                    value = resolvedValue;
                    return true;
                }
            }

            value = default;
            return false;
        }
    }

    public override void GenerateNodes(KdlNode generatorNode, INode scope)
    {
        if (generatorNode.Arguments.Count == 0 || generatorNode.Arguments[0] is not KdlString s || string.IsNullOrEmpty(s.Value))
        {
            throw new NodeParseException(generatorNode, "Expected string argument in foreach generator node for the module name or plugin ID.");
        }

        string nodeType = s.Value;

        if (nodeType == PluginNode.NodeTraits.Name)
        {
            foreach (PluginNode plugin in _context.ProjectService.GetAllPlugins())
            {
                if (DoesNodeMatch(generatorNode, plugin))
                {
                    GenerateNodes(generatorNode, scope, plugin);
                }
            }
        }
        else if (nodeType == ModuleNode.NodeTraits.Name)
        {
            foreach (ModuleNode plugin in _context.ProjectService.GetAllModules())
            {
                if (DoesNodeMatch(generatorNode, plugin))
                {
                    GenerateNodes(generatorNode, scope, plugin);
                }
            }
        }
        else
        {
            throw new NodeParseException(generatorNode, $"Unknown foreach generator node type '{nodeType}'. Expected '{PluginNode.NodeTraits.Name}' or '{ModuleNode.NodeTraits.Name}'.");
        }
    }

    private void GenerateNodes(KdlNode generatorNode, INode scope, INode contextNode)
    {
        ForeachReplacerContext replacerContext = new(_context, contextNode);

        foreach (KdlNode sourceChild in generatorNode.Children)
        {
            KdlNode generatedChild = GenerateNode(replacerContext, sourceChild);
            scope.Node.AddChild(generatedChild);

            if (_context.ProjectService.ParseNode(generatedChild, scope) is INode child)
            {
                scope.Children.Add(child);
            }
        }
    }

    private static KdlNode GenerateNode(ForeachReplacerContext replacerContext, KdlNode source)
    {
        string resolvedName = replacerContext.ReplaceTokens(source, source.Name);
        KdlNode result = new(resolvedName, source.Type) { SourceInfo = source.SourceInfo };

        // Copy properties, resolving any string values using the replacer
        for (int i = 0; i < source.Arguments.Count; ++i)
        {
            KdlValue value = source.Arguments[i];

            if (value is KdlString valueStr)
            {
                string resolvedValue = replacerContext.ReplaceTokens(source, valueStr.Value);
                result.Arguments.Add(new KdlString(resolvedValue, value.Type) { SourceInfo = value.SourceInfo });
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
                string resolvedValue = replacerContext.ReplaceTokens(source, valueStr.Value);
                result.Properties[key] = new KdlString(resolvedValue, valueStr.Type);
            }
            else
            {
                result.Properties[key] = value.Clone();
            }
        }

        // Recursively generate child nodes
        foreach (KdlNode child in source.Children)
        {
            result.AddChild(GenerateNode(replacerContext, child));
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
}
