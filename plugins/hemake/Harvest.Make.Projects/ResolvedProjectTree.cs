// Copyright Chad Engler

using Harvest.Common.Extensions;
using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;

namespace Harvest.Make.Projects;

public class ModuleDependency(DependenciesEntryNode entry, ModuleNode? resolvedModule)
{
    public string DependencyName => entry.DependencyName;
    public EDependencyKind Kind => entry.Kind;
    public bool IsExternal { get; set; } = entry.IsExternal;
    public bool IsWholeArchive { get; set; } = entry.IsWholeArchive;

    public ModuleNode? Module => resolvedModule;
}

public class ResolvedProjectTree
{
    private readonly HashSet<string> _activeTags = [];

    public ProjectContext ProjectContext { get; }
    public ProjectNode ProjectNode { get; }
    public IndexedNodeCollection IndexedNodes { get; }

    public ResolvedProjectTree(
        IProjectService projectService,
        ConfigurationNode configuration,
        PlatformNode platform,
        KdlNode sourceProjectNode)
    {
        IndexedNodes = new IndexedNodeCollection();

        ProjectContext = new ProjectContext()
        {
            ProjectService = projectService,
            Configuration = configuration,
            Platform = platform,
            BuildOutput = new BuildOutputNode(new KdlNode(BuildOutputNode.NodeTraits.Name) { SourceInfo = sourceProjectNode.SourceInfo }),
            Host = PlatformNodeTraits.GetHostPlatform(),
            Options = projectService.ProjectOptionValues,
            Tags = _activeTags,
        };

        NodeResolver resolver = new(ProjectContext);
        KdlNode projectNode = resolver.CreateResolvedNode(sourceProjectNode);
        ProjectNode = new ProjectNode(projectNode);

        resolver.ResolveDeferredNodes();

        IndexedNodes = resolver.IndexedNodes;
        ProjectContext.BuildOutput = GetMergedNode<BuildOutputNode>(ProjectNode.Node);

        ValidateNodeRecursive(ProjectNode.Node);
    }

    private void ValidateNodeRecursive(KdlNode node)
    {
        INodeTraits traits = ProjectContext.ProjectService.GetNodeTraits(node);
        traits.Validate(node);

        foreach (KdlNode child in node.Children)
        {
            ValidateNodeRecursive(child);
        }
    }

    public T GetMergedNode<T>(KdlNode? scope = null, bool searchDependencies = false)
        where T : class, INode
    {
        return GetMergedNode<T>(scope, (v) => true, searchDependencies);
    }

    public T GetMergedNode<T>(KdlNode? scope, Func<T, bool> filter, bool searchDependencies = false)
        where T : class, INode
    {
        scope ??= ProjectNode.Node;

        IEnumerable<T> nodesToMerge = GetNodes<T>(scope, searchDependencies);

        KdlNode mergedNode = new(T.NodeTraits.Name) { SourceInfo = scope.SourceInfo };
        T merged = INodeTraits.CreateNode<T>(mergedNode);

        if (nodesToMerge.Any())
        {
            foreach (T node in nodesToMerge)
            {
                if (filter(node))
                {
                    merged.MergeNode(ProjectContext, node.Node);
                }
            }
        }
        else
        {
            ResolveDefaultNode<T>(mergedNode, scope);
        }

        return merged;
    }

    public IEnumerable<T> GetNodes<T>(KdlNode scope, bool searchDependencies = false) where T : class, INode
    {
        foreach (KdlNode node in GetNodes(scope, T.NodeTraits, searchDependencies))
        {
            T semanticNode = INodeTraits.CreateNode<T>(node);
            yield return semanticNode;
        }
    }

    public IEnumerable<KdlNode> GetNodes(KdlNode scope, INodeTraits traits, bool searchDependencies = false)
    {
        // Optionally search depdenencies for the requested nodes, but only if the node is
        // allowed to be inherited through dependencies.
        ENodeDependencyInheritance inheritance = traits.DependencyInheritance;
        if (searchDependencies
            && inheritance != ENodeDependencyInheritance.None
            && traits.ValidScopes.Contains(PublicNode.NodeTraits.Name)
            && scope.Name == ModuleNode.NodeTraits.Name)
        {
            ModuleNode module = new(scope);
            List<ModuleDependency> dependencies = GetModuleDependencies(module, inheritance);
            foreach (ModuleDependency dep in dependencies)
            {
                if (dep.Module is ModuleNode dependencyModule)
                {
                    foreach (KdlNode node in GetNodes(dependencyModule.Node, traits, false))
                    {
                        if (IsWithinPublicNode(node))
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
            bool isRootNodeScope = traits.ValidScopes.Count == 0 && searchScope.Parent is null;
            if (isRootNodeScope || traits.ValidScopes.Contains(searchScope.Name))
            {
                stack.Push(searchScope);
            }

            searchScope = searchScope.Parent;
        } while (searchScope is not null);

        // Search each scope in order, from least specific to most specific.
        while (stack.Count > 0)
        {
            KdlNode scopeToCheck = stack.Pop();
            foreach (KdlNode child in scopeToCheck.Children)
            {
                if (child.Name == traits.Name)
                {
                    yield return child;
                }
                else if (child.Name == PublicNode.NodeTraits.Name && traits.ValidScopes.Contains(PublicNode.NodeTraits.Name))
                {
                    foreach (KdlNode publicChild in child.Children)
                    {
                        if (publicChild.Name == traits.Name)
                        {
                            yield return publicChild;
                        }
                    }
                }
            }

            if (scopeToCheck.Name == traits.Name)
            {
                yield return scopeToCheck;
            }
        }
    }

    public List<ModuleDependency> GetModuleDependencies(ModuleNode module, ENodeDependencyInheritance inheritance)
    {
        List<ModuleDependency> result = [];
        Dictionary<DependenciesEntryNode, int> indexMap = [];
        HashSet<(string ModuleName, bool PublicOnly)> visitedModules = [];

        GetModuleDependenciesInternal(module, result, indexMap, visitedModules, inheritance, false);

        return result;
    }

    private void ResolveDefaultNode<T>(KdlNode target, KdlNode scope)
        where T : class, INode
    {
        NodeTokenHandler handler = new(ProjectContext, IndexedNodes, scope);
        StringTokenReplacer replacer = new(handler);

        NodeResolver.ResolveDefaultNodeArguments(target, T.NodeTraits, replacer);
        NodeResolver.ResolveDefaultNodeProperties(target, T.NodeTraits, replacer);
    }

    private void GetModuleDependenciesInternal(
        ModuleNode module,
        List<ModuleDependency> result,
        Dictionary<DependenciesEntryNode, int> indexMap,
        HashSet<(string ModuleName, bool PublicOnly)> visitedModules,
        ENodeDependencyInheritance inheritance,
        bool publicOnly)
    {
        if (!visitedModules.Add((module.ModuleName, publicOnly)))
        {
            return;
        }

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
                            if (IndexedNodes.TryGetNode(entry.DependencyName, out ModuleNode? dependencyModule))
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
                                if (IndexedNodes.TryGetNode(entry.DependencyName, out ModuleNode? dependencyModule))
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
            GetModuleDependenciesInternal(dependencyModule, result, indexMap, visitedModules, inheritance, true);
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
}
