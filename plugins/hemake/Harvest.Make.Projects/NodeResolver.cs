// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public class NodeResolver(ProjectContext projectContext)
{
    public ProjectContext ProjectContext => projectContext;

    readonly struct DeferredNode(KdlNode target, KdlNode source)
    {
        public readonly KdlNode target = target;
        public readonly KdlNode source = source;
    }

    private readonly List<DeferredNode> _deferredNodes = [];

    private class ScopeTags
    {
        public readonly List<string> tags = [];
        public int refCount = 0;
    }

    private readonly Dictionary<KdlNode, ScopeTags> _tagsForScope = [];

    private readonly IndexedNodeCollection _indexedNodes = new();
    public IndexedNodeCollection IndexedNodes => _indexedNodes;

    public static KdlValue CreateResolvedNodeValue(KdlValue value, NodeValueDef def, StringTokenReplacer replacer)
    {
        if (value is KdlString valueString)
        {
            string resolvedValue = replacer.ReplaceTokens(valueString.Value);

            if (def is NodeValueDef_Path && !string.IsNullOrEmpty(resolvedValue))
            {
                if (!Path.IsPathRooted(resolvedValue))
                {
                    if (Path.GetDirectoryName(value.SourceInfo.FilePath) is string fileDir)
                    {
                        resolvedValue = Path.GetFullPath(resolvedValue, fileDir);
                    }
                    else
                    {
                        resolvedValue = Path.GetFullPath(resolvedValue);
                    }
                }
            }

            return new KdlString(resolvedValue, value.Type) { SourceInfo = value.SourceInfo };
        }
        else
        {
            // For non-string values, just clone them directly
            return value.Clone();
        }
    }

    public static void ResolveDefaultNodeArguments(KdlNode target, INodeTraits traits, StringTokenReplacer replacer)
    {
        for (int i = 0; i < traits.ArgumentDefs.Count; ++i)
        {
            NodeValueDef def = traits.ArgumentDefs[i];
            if (def.DefaultValue is KdlNull)
            {
                continue;
            }

            target.Arguments.Add(CreateResolvedNodeValue(def.DefaultValue, def, replacer));
        }
    }

    public static void ResolveNodeArguments(KdlNode target, KdlNode source, INodeTraits traits, StringTokenReplacer replacer)
    {
        for (int i = 0; i < source.Arguments.Count; ++i)
        {
            KdlValue value = source.Arguments[i];
            NodeValueDef def = traits.ArgumentDefs[i];

            if (i < target.Arguments.Count)
            {
                target.Arguments[i] = CreateResolvedNodeValue(value, def, replacer);
            }
            else
            {
                target.Arguments.Add(CreateResolvedNodeValue(value, def, replacer));
            }
        }
    }

    public static void ResolveNodeProperties(KdlNode target, KdlNode source, INodeTraits traits, StringTokenReplacer replacer)
    {
        foreach ((string key, KdlValue value) in source.Properties)
        {
            if (!traits.PropertyDefs.TryGetValue(key, out NodeValueDef? def))
            {
                // Allow passthrough properties for node-specific metadata (for example plugin metadata keys).
                // Resolve tokens for string values, but skip path normalization without an explicit value def.
                if (value is KdlString valueString)
                {
                    string resolvedValue = replacer.ReplaceTokens(valueString.Value);
                    target.Properties[key] = new KdlString(resolvedValue, value.Type) { SourceInfo = value.SourceInfo };
                }
                else
                {
                    target.Properties[key] = value.Clone();
                }

                continue;
            }

            target.Properties[key] = CreateResolvedNodeValue(value, def, replacer);
        }
    }

    public static void ResolveDefaultNodeProperties(KdlNode target, INodeTraits traits, StringTokenReplacer replacer)
    {
        foreach ((string key, NodeValueDef def) in traits.PropertyDefs)
        {
            if (def.DefaultValue is KdlNull)
            {
                continue;
            }

            target.Properties[key] = CreateResolvedNodeValue(def.DefaultValue, def, replacer);
        }
    }

    public void AddActiveTagForScope(KdlNode scope, string tagName)
    {
        if (string.IsNullOrEmpty(tagName))
        {
            return;
        }

        if (!_tagsForScope.TryGetValue(scope, out ScopeTags? scopeTags))
        {
            scopeTags = new ScopeTags();
            _tagsForScope.Add(scope, scopeTags);
        }

        if (ProjectContext.Tags.Add(tagName))
        {
            scopeTags.tags.Add(tagName);
        }
    }

    public KdlNode CreateResolvedNode(KdlNode source, bool includeChildren = true)
    {
        NodeTokenHandler handler = new(ProjectContext, _indexedNodes, source);
        StringTokenReplacer replacer = new(handler);

        string resolvedName = replacer.ReplaceTokens(source.Name);
        KdlNode result = new(resolvedName, source.Type) { SourceInfo = source.SourceInfo };

        INodeTraits traits = ProjectContext.ProjectService.GetNodeTraits(source);

        // First resolve defaults from the node traits
        ResolveDefaultNodeArguments(result, traits, replacer);
        ResolveDefaultNodeProperties(result, traits, replacer);

        // Then resolve any arguments/properties from the source node, which will override defaults
        ResolveNodeArguments(result, source, traits, replacer);
        ResolveNodeProperties(result, source, traits, replacer);

        if (resolvedName == ProjectNode.NodeTraits.Name)
        {
            ProjectContext.Project = new ProjectNode(result);
        }
        else if (resolvedName == PluginNode.NodeTraits.Name)
        {
            PluginNode plugin = new(result);
            ProjectContext.Plugin = plugin;

            if (!_indexedNodes.TryAddNode(plugin.PluginName, plugin))
            {
                throw new NodeParseException(source, $"Encountered duplicate plugin name '{plugin.PluginName}'. Plugin names must be unique. Previously defined in: {_indexedNodes.GetNode<PluginNode>(plugin.PluginName).Node.SourceInfo.ToErrorString()}");
            }
        }
        else if (resolvedName == ModuleNode.NodeTraits.Name)
        {
            ModuleNode module = new(result);
            ProjectContext.Module = module;

            if (!_indexedNodes.TryAddNode(module.ModuleName, module))
            {
                throw new NodeParseException(source, $"Encountered duplicate module name '{module.ModuleName}'. Module names must be unique. Previously defined in: {_indexedNodes.GetNode<ModuleNode>(module.ModuleName).Node.SourceInfo.ToErrorString()}");
            }
        }
        else if (resolvedName == BuildOutputNode.NodeTraits.Name)
        {
            ProjectContext.BuildOutput = new BuildOutputNode(result);
        }

        if (includeChildren)
        {
            ResolveNodeChildren(result, source, traits, replacer);
        }

        if (resolvedName == PluginNode.NodeTraits.Name)
        {
            ProjectContext.Plugin = null;
        }
        else if (resolvedName == ModuleNode.NodeTraits.Name)
        {
            ProjectContext.Module = null;
        }

        return result;
    }

    public void ResolveNodeChildren(KdlNode target, KdlNode source, INodeTraits traits, StringTokenReplacer replacer)
    {
        if (!_tagsForScope.TryGetValue(target, out ScopeTags? scopeTags))
        {
            scopeTags = new ScopeTags();
            _tagsForScope.Add(target, scopeTags);
        }

        ++scopeTags.refCount;

        foreach (KdlNode child in source.Children)
        {
            // Generators and extensions are deferred until after the initial tree is built
            if (child.Name.StartsWith(':') || child.Name.StartsWith('+'))
            {
                _deferredNodes.Add(new DeferredNode(target, child));
                continue;
            }

            INodeTraits childTraits = ProjectContext.ProjectService.GetNodeTraits(child);
            if (!childTraits.TryResolveChild(target, child, replacer, this, out KdlNode? resolvedChild))
            {
                resolvedChild = CreateResolvedNode(child);
            }

            if (resolvedChild is not null)
            {
                target.AddChild(resolvedChild);
            }
        }

        // Remove any tags we added from this scope after processing its children
        if (--scopeTags.refCount == 0)
        {
            foreach (string tag in scopeTags.tags)
            {
                ProjectContext.Tags.Remove(tag);
            }
        }
    }

    public void ResolveDeferredNodes()
    {
        do
        {
            // Make a copy of the nodes to process so that resolving them can modify the original list
            // while we're iterating.
            List<DeferredNode> list = [.. _deferredNodes];
            _deferredNodes.Clear();

            foreach (DeferredNode deferred in list)
            {
                if (deferred.source.Name.StartsWith('+'))
                {
                    ResolveExtensionNode(deferred.source);
                }
                else if (deferred.source.Name.StartsWith(':'))
                {
                    ResolveGeneratorNode(deferred.target, deferred.source);
                }
                else
                {
                    throw new NodeParseException(deferred.source, "Deferred node is neither an extension nor a generator. This is a bug.");
                }
            }
        } while (_deferredNodes.Count > 0);
    }

    public void ResolveExtensionNode(KdlNode extensionNode)
    {
        if (!extensionNode.TryGetValue(0, out string? nodeId) || string.IsNullOrEmpty(nodeId))
        {
            throw new NodeParseException(extensionNode, "Expected string argument in extension node for the module name or plugin ID.");
        }

        NodeTokenHandler handler = new(ProjectContext, _indexedNodes, extensionNode);
        StringTokenReplacer replacer = new(handler);

        string nodeName = extensionNode.Name[1..];
        if (nodeName == PluginNode.NodeTraits.Name)
        {
            if (_indexedNodes.TryGetNode(nodeId, out PluginNode? plugin))
            {
                ResolveNodeArguments(plugin.Node, extensionNode, PluginNode.NodeTraits, replacer);
                ResolveNodeProperties(plugin.Node, extensionNode, PluginNode.NodeTraits, replacer);
                ResolveNodeChildren(plugin.Node, extensionNode, PluginNode.NodeTraits, replacer);
            }
            else if (extensionNode.TryGetValue("required", out bool isRequired) && isRequired)
            {
                throw new NodeParseException(extensionNode, $"Failed to resolve required plugin '{nodeId}'.");
            }
        }
        else if (nodeName == ModuleNode.NodeTraits.Name)
        {
            if (_indexedNodes.TryGetNode(nodeId, out ModuleNode? module))
            {
                ResolveNodeArguments(module.Node, extensionNode, ModuleNode.NodeTraits, replacer);
                ResolveNodeProperties(module.Node, extensionNode, ModuleNode.NodeTraits, replacer);
                ResolveNodeChildren(module.Node, extensionNode, ModuleNode.NodeTraits, replacer);
            }
            else if (extensionNode.TryGetValue("required", out bool isRequired) && isRequired)
            {
                throw new NodeParseException(extensionNode, $"Failed to resolve required module '{nodeId}'.");
            }
        }
        else
        {
            throw new NodeParseException(extensionNode, $"Unknown extension node type '{nodeName}'. Expected '{PluginNode.NodeTraits.Name}' or '{ModuleNode.NodeTraits.Name}'.");
        }
    }

    public void ResolveGeneratorNode(KdlNode target, KdlNode generatorNode)
    {
        INodeGenerator generator = ProjectContext.ProjectService.CreateGeneratorForNode(generatorNode, this);
        generator.GenerateNodes(target, generatorNode);
    }
}
