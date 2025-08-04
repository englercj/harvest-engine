// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Nodes;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Make.Projects.NodeGenerators;

public class ForeachNodeGenerator(ProjectContext context) : NodeGeneratorBase(context)
{
    private class EntryContext : ProjectContext
    {
        public INode EntryNode { get; }

        public EntryContext(ProjectContext parentContext, INode entryNode)
            : base(parentContext.ProjectService)
        {
            Plugin = parentContext.Plugin;
            Module = parentContext.Module;
            Configuration = parentContext.Configuration;
            Platform = parentContext.Platform;
            Host = parentContext.Host;
            Options = parentContext.Options;
            Tags = parentContext.Tags;

            EntryNode = entryNode;
        }
    }

    public static string GeneratorName => "foreach";

    public override void Resolve(KdlNode generatorNode, INode scope)
    {
        if (generatorNode.Arguments.Count == 0 || generatorNode.Arguments[0] is not KdlString s || string.IsNullOrEmpty(s.Value))
        {
            throw new NodeParseException(generatorNode, "Expected string argument in foreach generator node for the module name or plugin ID.");
        }

        string nodeType = s.Value;

        if (nodeType == PluginNode.NodeName)
        {
            foreach (PluginNode plugin in _context.ProjectService.GetAllPlugins())
            {
                if (DoesNodeMatch(generatorNode, plugin))
                {
                    GenerateNodes(generatorNode, scope, plugin);
                }
            }
        }
        else if (nodeType == ModuleNode.NodeName)
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
            throw new NodeParseException(generatorNode, $"Unknown foreach generator node type '{nodeType}'. Expected '{PluginNode.NodeName}' or '{ModuleNode.NodeName}'.");
        }
    }

    private void GenerateNodes(KdlNode generatorNode, INode scope, INode context)
    {
        // TODO: Go through each child node of the generatorNode and clone it into the scope
        // Make sure to replace any tokens using the `_entry` context in the child nodes to refer
        // to the context node paramter. Other tokens should be left alone to be resolved later.

        NodeTokenReplacer replacer = new(new Dictionary<string, NodeTokenResolver>()
        {
            { "_entry", EntryTokenResolver },
        });

        EntryContext replaceContext = new(_context, context);

        foreach (KdlNode rawChild in generatorNode.Children)
        {
            KdlNode clonedRawChild = rawChild.Clone();
            scope.Node.AddChild(clonedRawChild);

            if (_context.ProjectService.ParseNode(clonedRawChild, scope) is INode child)
            {
                string resolvedName = replacer.ReplaceTokens(replaceContext, child, child.Node.Name);
                if (resolvedName != child.Node.Name)
                {
                    child.Node.SourceInfo = child.Node.SourceInfo with { Name = resolvedName };
                    child.Node = new KdlNode(resolvedName, child.Node.Type)
                    {
                        SourceInfo = child.Node.SourceInfo,
                    };
                }

                for (int i = 0; i < child.Node.Arguments.Count; ++i)
                {
                    KdlValue value = child.Node.Arguments[i];

                    if (value is KdlString valueStr)
                    {
                        string resolvedValue = replacer.ReplaceTokens(_context, child, valueStr.Value);
                        child.Node.Arguments[i] = new KdlString(resolvedValue, value.Type) { SourceInfo = value.SourceInfo };
                    }
                }

                foreach ((string key, KdlValue value) in child.Node.Properties)
                {
                    if (value is KdlString valueStr)
                    {
                        string resolvedValue = replacer.ReplaceTokens(_context, child, valueStr.Value);
                        Node.Properties[key] = new KdlString(resolvedValue, valueStr.Type);
                    }
                }

                scope.Children.Add(child);
            }
        }
    }

    private static bool EntryTokenResolver(ProjectContext projectContext, string contextName, string propertyName, [MaybeNullWhen(false)] out string value)
    {
        Debug.Assert(contextName == "_entry");
        Debug.Assert(projectContext is EntryContext);

        EntryContext entryContext = (EntryContext)projectContext;

        // TODO: Treat _entry as if it was named `entryConext.EntryNode.Node.Name` and do the resolve on that.
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
