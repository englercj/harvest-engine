// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Extensions;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

public class ResolvedProjectTree
{
    private readonly HashSet<string> _activeTags = [];

    public ProjectContext ProjectContext { get; }
    public ProjectNode ProjectNode { get; }

    readonly struct DeferredNode(KdlNode source, KdlNode scope)
    {
        public readonly KdlNode source = source;
        public readonly KdlNode scope = scope;
    }

    private readonly List<DeferredNode> _deferredNodes = [];

    private readonly SortedDictionary<string, PluginNode> _plugins = [];
    private readonly SortedDictionary<string, ModuleNode> _modules = [];

    public ResolvedProjectTree(
        IProjectService projectService,
        ConfigurationNode configuration,
        PlatformNode platform,
        KdlNode sourceProjectNode)
    {
        ProjectContext = new ProjectContext()
        {
            ProjectService = projectService,
            Configuration = configuration,
            Platform = platform,
            Host = PlatformNodeTraits.GetHostPlatform(),
            Options = projectService.ProjectOptionValues,
            Tags = _activeTags,
        };

        ProjectNode = new ProjectNode(CreateResolvedNode(sourceProjectNode));

        UpdateNodeCaches();

        // Resolve any deferred nodes we encountered during parse
        while (_deferredNodes.Count > 0)
        {
            // Make a copy of the deferred nodes to process, then clear the list so that
            // any new deferred nodes added during processing will be handled in the next iteration.
            List<DeferredNode> copy = [.. _deferredNodes];
            _deferredNodes.Clear();

            ResolveDeferredNodes(copy);
            UpdateNodeCaches();
        }

        // TODO: Validate the final tree?
    }

    public IEnumerable<ModuleNode> GetAllModules()
    {
        return _modules.Values;
    }

    public ModuleNode? TryGetModuleByName(string moduleName)
    {
        if (_modules.TryGetValue(moduleName, out ModuleNode? module))
        {
            return module;
        }
        return null;
    }

    public IEnumerable<PluginNode> GetAllPlugins()
    {
        return _plugins.Values;
    }

    public PluginNode? TryGetPluginByName(string pluginName)
    {
        if (_plugins.TryGetValue(pluginName, out PluginNode? plugin))
        {
            return plugin;
        }
        return null;
    }

    public T GetMergedNode<T>(KdlNode? scope = null, bool searchDependencies = true) where T : class, INode
    {
        return GetMergedNode<T>(scope, (v) => true, searchDependencies);
    }

    public T GetMergedNode<T>(KdlNode? scope, Func<T, bool> filter, bool searchDependencies = true) where T : class, INode
    {
        scope ??= ProjectNode.Node;

        IEnumerable<T> nodesToMerge = GetNodes(scope, filter, searchDependencies);

        KdlNode result = new(T.NodeTraits.Name) { SourceInfo = scope.SourceInfo };

        if (nodesToMerge.Any())
        {
            foreach (T node in nodesToMerge)
            {
                resolved.MergeAndResolve(context, node);
            }
        }
        else
        {
            // TODO:
            resolved.ResolveDefaults(context);
        }

        return ProjectContext.ProjectService.CreateSemanticNode<T>(result);
    }

    public IEnumerable<T> GetNodes<T>(KdlNode scope, bool searchDependencies) where T : class, INode
    {
        return GetNodes<T>(scope, (v) => true, searchDependencies);
    }

    public IEnumerable<T> GetNodes<T>(KdlNode scope, Func<T, bool> filter, bool searchDependencies) where T : class, INode
    {
        // Optionally search depdenencies for the requested nodes, but only if the node is
        // allowed to be inherited through dependencies.
        ENodeDependencyInheritance inheritance = T.NodeTraits.DependencyInheritance;
        if (searchDependencies
            && inheritance != ENodeDependencyInheritance.None
            && T.NodeTraits.ValidScopes.Contains(PublicNode.NodeTraits.Name)
            && scope.Name == ModuleNode.NodeTraits.Name)
        {
            ModuleNode module = new(scope);
            List<ModuleDependency> dependencies = GetModuleDependencies(module, inheritance);
            foreach (ModuleDependency dep in dependencies)
            {
                if (dep.Module is ModuleNode dependencyModule)
                {
                    foreach (T node in GetNodes(dependencyModule.Node, filter, searchDependencies))
                    {
                        if (IsWithinPublicNode(node.Node))
                        {
                            yield return node;
                        }
                    }
                }
            }
        }

        Stack<KdlNode> stack = new();

        // Walk up the scope chain and find all the scopes we need to check, this will place the
        // least specific scope at the top of the stack.
        KdlNode? searchScope = scope;
        do
        {
            if (T.NodeTraits.ValidScopes.Contains(searchScope.Name))
            {
                stack.Push(scope);
            }

            searchScope = searchScope.Parent;
        } while (searchScope is not null);

        // Search each scope in order, from least specific to most specific.
        while (stack.Count > 0)
        {
            KdlNode scopeToCheck = stack.Pop();
            foreach (KdlNode child in scopeToCheck.Children)
            {
                if (child.Name == T.NodeTraits.Name)
                {
                    T instance = ProjectContext.ProjectService.CreateSemanticNode<T>(child);
                    if (filter(instance))
                    {
                        yield return instance;
                    }
                }
            }

            if (scopeToCheck.Name == T.NodeTraits.Name)
            {
                T instance = ProjectContext.ProjectService.CreateSemanticNode<T>(scopeToCheck);
                if (filter(instance))
                {
                    yield return instance;
                }
            }
        }
    }

    public List<ModuleDependency> GetModuleDependencies(ModuleNode module, ENodeDependencyInheritance inheritance)
    {
        List<ModuleDependency> result = [];
        Dictionary<DependenciesEntryNode, int> indexMap = [];

        GetModuleDependenciesInternal(module, result, indexMap, inheritance, false);

        return result;
    }

    private void GetModuleDependenciesInternal(
        ModuleNode module,
        List<ModuleDependency> result,
        Dictionary<DependenciesEntryNode, int> indexMap,
        ENodeDependencyInheritance inheritance,
        bool publicOnly)
    {
        List<ModuleNode> modulesToRecurse = [];

        // First add all our immediate dependencies to the list
        foreach (DependenciesNode dependencies in GetNodes<DependenciesNode>(module.Node, false))
        {
            if (publicOnly && !IsWithinPublicNode(dependencies.Node))
            {
                continue;
            }

            foreach (DependenciesEntryNode entry in dependencies.Entries)
            {
                if (indexMap.TryGetValue(entry, out int index))
                {
                    ModuleDependency moduleDependency = result[index];
                    moduleDependency.IsExternal |= entry.IsExternal;
                    moduleDependency.IsWholeArchive |= entry.IsWholeArchive;
                }
                else
                {
                    bool shouldInclude = false;
                    ModuleNode? resolvedDepdencyModule = null;
                    switch (entry.Kind)
                    {
                        case EDependencyKind.Default:
                        {
                            if (TryGetModuleByName(entry.DependencyName) is ModuleNode dependencyModule)
                            {
                                resolvedDepdencyModule = dependencyModule;

                                switch (dependencyModule.Kind)
                                {
                                    case EModuleKind.AppConsole:
                                    case EModuleKind.AppWindowed:
                                    case EModuleKind.LibHeader:
                                    {
                                        shouldInclude = inheritance.HasAnyFlag(
                                            ENodeDependencyInheritance.Order
                                            | ENodeDependencyInheritance.Include);
                                        break;
                                    }
                                    case EModuleKind.Content:
                                    {
                                        shouldInclude = inheritance.HasFlag(ENodeDependencyInheritance.Content);
                                        break;
                                    }
                                    case EModuleKind.Custom:
                                    {
                                        shouldInclude = inheritance.HasFlag(ENodeDependencyInheritance.Order);
                                        break;
                                    }
                                    case EModuleKind.LibStatic:
                                    case EModuleKind.LibShared:
                                    {
                                        shouldInclude = inheritance.HasAnyFlag(
                                            ENodeDependencyInheritance.Link
                                            | ENodeDependencyInheritance.Order
                                            | ENodeDependencyInheritance.Include);
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                throw new NodeParseException(entry.Node, $"Failed to resolve dependency '{entry.DependencyName}' for module '{module.ModuleName}'.");
                            }
                            break;
                        }
                        case EDependencyKind.Include:
                        {
                            shouldInclude = inheritance.HasFlag(ENodeDependencyInheritance.Include);
                            break;
                        }
                        case EDependencyKind.Link:
                        {
                            shouldInclude = inheritance.HasFlag(ENodeDependencyInheritance.Link);
                            break;
                        }
                        case EDependencyKind.Order:
                        {
                            shouldInclude = inheritance.HasFlag(ENodeDependencyInheritance.Order);
                            break;
                        }
                        case EDependencyKind.File:
                        case EDependencyKind.System:
                        {
                            shouldInclude = inheritance.HasFlag(ENodeDependencyInheritance.Link);
                            break;
                        }
                    }

                    if (shouldInclude)
                    {
                        if (entry.Kind != EDependencyKind.File && entry.Kind != EDependencyKind.System)
                        {
                            if (resolvedDepdencyModule is null)
                            {
                                if (TryGetModuleByName(entry.DependencyName) is ModuleNode dependencyModule)
                                {
                                    resolvedDepdencyModule = dependencyModule;
                                }
                                else
                                {
                                    throw new NodeParseException(entry.Node, $"Failed to resolve dependency '{entry.DependencyName}' for module '{module.ModuleName}'.");
                                }
                            }

                            if (resolvedDepdencyModule is not null)
                            {
                                modulesToRecurse.Add(resolvedDepdencyModule);
                            }
                        }


                        indexMap.Add(entry, result.Count);
                        result.Add(new ModuleDependency(entry, resolvedDepdencyModule));
                    }
                }
            }
        }

        // Then add all the public dependencies of our dependencies
        foreach (ModuleNode dependencyModule in modulesToRecurse)
        {
            GetModuleDependenciesInternal(dependencyModule, result, indexMap, inheritance, true);
        }
    }

    private static bool IsWithinPublicNode(KdlNode node)
    {
        KdlNode? scope = node.Parent;

        while (scope is not null)
        {
            if (scope.Name == PublicNode.NodeTraits.Name)
            {
                return true;
            }

            scope = scope.Parent;
        }

        return false;
    }

    #region Node Resolution

    private KdlNode CreateResolvedNode(KdlNode source)
    {
        NodeTokenHandler handler = new(ProjectContext, source);
        StringTokenReplacer replacer = new(handler);

        string resolvedName = replacer.ReplaceTokens(source.Name);
        KdlNode result = new(resolvedName, source.Type) { SourceInfo = source.SourceInfo };

        ResolveNodeArguments(result, source, replacer);
        ResolveNodeProperties(result, source, replacer);
        ResolveNodeChildren(result, source, replacer);

        return result;
    }

    private static void ResolveNodeArguments(KdlNode dest, KdlNode source, StringTokenReplacer replacer)
    {
        foreach (KdlValue value in source.Arguments)
        {
            if (value is KdlString valueStr)
            {
                string resolvedValue = replacer.ReplaceTokens(valueStr.Value);
                dest.Arguments.Add(new KdlString(resolvedValue, valueStr.Type) { SourceInfo = value.SourceInfo });
            }
            else
            {
                // For non-string values, just clone them directly
                dest.Arguments.Add(value.Clone());
            }
        }
    }

    private static void ResolveNodeProperties(KdlNode dest, KdlNode source, StringTokenReplacer replacer)
    {
        foreach ((string key, KdlValue value) in source.Properties)
        {
            if (value is KdlString valueStr)
            {
                string resolvedValue = replacer.ReplaceTokens(valueStr.Value);
                dest.Properties[key] = new KdlString(resolvedValue, valueStr.Type) { SourceInfo = value.SourceInfo };
            }
            else
            {
                dest.Properties[key] = value.Clone();
            }
        }
    }

    private void ResolveNodeChildren(KdlNode dest, KdlNode source, StringTokenReplacer replacer)
    {
        List<string> uniqueScopeTags = [];

        foreach (KdlNode child in source.Children)
        {
            // Generators and extensions are deferred until after the initial tree is built
            if (source.Name.StartsWith(':') || source.Name.StartsWith('+'))
            {
                _deferredNodes.Add(new DeferredNode(source, dest));
            }
            // When nodes need special handling to only include their children if active
            else if (child.Name == WhenNode.NodeTraits.Name)
            {
                // Clone and resolve the when node so that tokens in the arguments/properties are resolved before checking if it's active
                KdlNode clonedNode = child.Clone(false);
                ResolveNodeArguments(clonedNode, child, replacer);
                ResolveNodeProperties(clonedNode, child, replacer);

                WhenNode whenNode = new(clonedNode);
                if (whenNode.IsActive(ProjectContext))
                {
                    ResolveNodeChildren(dest, whenNode.Node, replacer);
                }
            }
            // Tag nodes also need  Add any tags from tags nodes to the active tag set
            else if (child.Name == TagsNode.NodeTraits.Name)
            {
                KdlNode tagsNode = CreateResolvedNode(child);
                dest.AddChild(tagsNode);

                TagsNode tags = new(tagsNode);
                foreach (TagsEntryNode entry in tags.Entries)
                {
                    if (!string.IsNullOrEmpty(entry.TagName) && _activeTags.Add(entry.TagName))
                    {
                        uniqueScopeTags.Add(entry.TagName);
                    }
                }
            }
            // All other children are resolved normally
            else
            {
                dest.AddChild(CreateResolvedNode(child));
            }
        }

        // Remove any tags we added from this scope after processing its children
        foreach (string tag in uniqueScopeTags)
        {
            _activeTags.Remove(tag);
        }
    }

    private void ResolveDeferredNodes(List<DeferredNode> list)
    {
        foreach (DeferredNode deferred in list)
        {
            if (deferred.source.Name.StartsWith('+'))
            {
                ResolveExtensionNode(deferred.source);
            }
            else if (deferred.source.Name.StartsWith(':'))
            {
                ResolveGeneratorNode(deferred.source, deferred.scope);
            }
            else
            {
                throw new NodeParseException(deferred.source, "Deferred node is neither an extension nor a generator. This is a bug.");
            }
        }
    }

    private void ResolveExtensionNode(KdlNode extensionNode)
    {
        if (!extensionNode.TryGetArgumentValue(0, out string? nodeId) || string.IsNullOrEmpty(nodeId))
        {
            throw new NodeParseException(extensionNode, "Expected string argument in extension node for the module name or plugin ID.");
        }

        string nodeName = extensionNode.Name[1..];
        if (nodeName == PluginNode.NodeTraits.Name)
        {
            if (_plugins.TryGetValue(nodeId, out PluginNode? plugin))
            {
                extensionNode.CopyTo(plugin.Node, false);
            }
            else if (extensionNode.TryGetPropertyValue("required", out bool isRequired) && isRequired)
            {
                throw new NodeParseException(extensionNode, $"Failed to resolve required plugin '{nodeId}'.");
            }
        }
        else if (nodeName == ModuleNode.NodeTraits.Name)
        {
            if (_modules.TryGetValue(nodeId, out ModuleNode? module))
            {
                extensionNode.CopyTo(module.Node, false);
            }
            else if (extensionNode.TryGetPropertyValue("required", out bool isRequired) && isRequired)
            {
                throw new NodeParseException(extensionNode, $"Failed to resolve required module '{nodeId}'.");
            }
        }
        else
        {
            throw new NodeParseException(extensionNode, $"Unknown extension node type '{nodeName}'. Expected '{PluginNode.NodeTraits.Name}' or '{ModuleNode.NodeTraits.Name}'.");
        }
    }

    private void ResolveGeneratorNode(KdlNode generatorNode, KdlNode scope)
    {
        INodeGenerator generator = ProjectContext.ProjectService.CreateGeneratorForNode(generatorNode);
        generator.GenerateNodes(generatorNode, scope);
    }

    #endregion

    private void UpdateNodeCaches()
    {
        _plugins.Clear();
        _modules.Clear();

        foreach (KdlNode node in ProjectNode.Node.GetAllDescendants())
        {
            if (node.Name == PluginNode.NodeTraits.Name)
            {
                PluginNode plugin = new(node);
                if (!_plugins.TryAdd(plugin.PluginName, plugin))
                {
                    throw new NodeValidationException(plugin, $"Encountered duplicate plugin name '{plugin.PluginName}'. Plugin names must be unique. Previously defined in: {_plugins[plugin.PluginName].Node.SourceInfo.ToErrorString()}");
                }
            }
            else if (node.Name == ModuleNode.NodeTraits.Name)
            {
                ModuleNode module = new(node);
                if (!_modules.TryAdd(module.ModuleName, module))
                {
                    throw new NodeValidationException(module, $"Encountered duplicate module name '{module.ModuleName}'. Module names must be unique. Previously defined in: {_modules[module.ModuleName].Node.SourceInfo.ToErrorString()}");
                }
            }
        }
    }
}
