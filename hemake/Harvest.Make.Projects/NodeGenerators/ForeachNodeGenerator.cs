// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using System.Text;

namespace Harvest.Make.Projects.NodeGenerators;

internal class ForeachNodeGeneratorTraits : NodeGeneratorBaseTraits
{
    public override string Name => "foreach";

    public override INodeGenerator CreateGenerator(IProjectService projectService, NodeResolver resolver) =>
        new ForeachNodeGenerator(projectService, resolver);
}

internal class ForeachNodeGenerator(IProjectService projectService, NodeResolver resolver) : NodeGeneratorBase<ForeachNodeGeneratorTraits>(projectService, resolver)
{
    public override void GenerateNodes(KdlNode target, KdlNode generatorNode)
    {
        if (!generatorNode.TryGetValue(0, out string? nodeType) || string.IsNullOrEmpty(nodeType))
        {
            throw new NodeParseException(generatorNode, "Expected string argument in foreach generator node for the module name or plugin ID.");
        }

        if (nodeType == PluginNode.NodeTraits.Name)
        {
            foreach (PluginNode plugin in _resolver.IndexedNodes.GetAllNodes<PluginNode>())
            {
                if (DoesNodeMatch(generatorNode, plugin))
                {
                    GenerateNodes(target, generatorNode, $"plugin[{plugin.PluginName}]");
                }
            }
        }
        else if (nodeType == ModuleNode.NodeTraits.Name)
        {
            foreach (ModuleNode module in _resolver.IndexedNodes.GetAllNodes<ModuleNode>())
            {
                if (DoesNodeMatch(generatorNode, module))
                {
                    GenerateNodes(target, generatorNode, $"module[{module.ModuleName}]");
                }
            }
        }
        else
        {
            throw new NodeParseException(generatorNode, $"Unknown foreach generator node type '{nodeType}'. Expected '{PluginNode.NodeTraits.Name}' or '{ModuleNode.NodeTraits.Name}'.");
        }
    }

    private void GenerateNodes(KdlNode target, KdlNode generatorNode, string contextName)
    {
        TokenHandler handler = new(contextName);
        StringTokenReplacer replacer = new(handler);

        foreach (KdlNode source in generatorNode.Children)
        {
            KdlNode generated = GenerateNode(source, replacer);

            if (generated.Name.StartsWith('+'))
            {
                _resolver.ResolveExtensionNode(generated);
                continue;
            }

            if (generated.Name.StartsWith(':'))
            {
                _resolver.ResolveGeneratorNode(target, generated);
                continue;
            }

            // Generated nodes need to inherit the scope where the foreach appears so trait
            // resolution can infer set-entry node types from their parent scope.
            target.AddChild(generated);

            KdlNode resolved;
            try
            {
                resolved = _resolver.CreateResolvedNode(generated, includeChildren: true);
            }
            finally
            {
                target.RemoveChild(generated);
            }

            target.AddChild(resolved);
        }
    }

    private static KdlNode GenerateNode(KdlNode source, StringTokenReplacer replacer)
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
            result.AddChild(GenerateNode(child, replacer));
        }

        return result;
    }

    private static bool DoesNodeMatch<T>(KdlNode generatorNode, T candidate) where T : class, INode
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

                if (argIndex >= 0 && argIndex < candidate.Node.Arguments.Count)
                {
                    if (!DoesValueMatch(value, candidate.Node.Arguments[argIndex]))
                    {
                        return false;
                    }

                    continue;
                }

                // If there is no argument at that index, it doesn't match
                return false;
            }

            if (!candidate.Node.Properties.TryGetValue(key, out KdlValue? candidateValue))
            {
                return false;
            }

            if (!DoesValueMatch(value, candidateValue))
            {
                return false;
            }

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

                int relativeIndex = tokenContext.ContextNameCapture.Index - tokenContext.TokenMatch.Index;

                StringBuilder sb = new();
                sb.Append(tokenContext.Token[..relativeIndex]);
                sb.Append(resolvedContextName);
                sb.Append(tokenContext.Token[(relativeIndex + tokenContext.ContextNameCapture.Length)..]);

                return sb.ToString();
            }

            return tokenContext.Token;
        }
    }
}
