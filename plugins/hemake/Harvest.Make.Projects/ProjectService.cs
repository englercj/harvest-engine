// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Attributes;
using Harvest.Make.Extensions;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.FileSystemGlobbing;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects;

[Service<IProjectService>]
public class ProjectService : IProjectService
{
    private readonly Dictionary<string, Type> _nodeTypes = [];
    private readonly Dictionary<string, Type> _nodeGenerators = [];
    private readonly Dictionary<string, NodeTokenResolver> _tokenResolvers = [];
    private readonly Dictionary<string, NodeTokenTransformer> _tokenTransformers = new(NodeTokenReplacer.DefaultTransformers);
    private readonly Dictionary<string, KdlDocument> _kdlFiles = [];
    private readonly Dictionary<string, PluginNode> _plugins = [];
    private readonly Dictionary<string, ModuleNode> _modules = [];
    private readonly List<ProjectOption> _projectOptions = [];
    private readonly List<(KdlNode RawNode, INode Scope)> _deferredNodes = [];
    private KdlDocument? _projectDocument;
    private ProjectNode? _projectNode;
    private NodeTokenReplacer? _tokenReplacer;

    public string ProjectPath { get; private set; } = string.Empty;
    public KdlDocument ProjectDocument => _projectDocument ?? throw new Exception("Project not loaded. Call LoadProject() first.");
    public ProjectNode ProjectNode => _projectNode ?? throw new Exception("Project not parsed. Call ParseProject() first.");
    public IReadOnlyList<ProjectOption> ProjectOptions => _projectOptions;
    public NodeTokenReplacer TokenReplacer => _tokenReplacer ??= new(_tokenResolvers, _tokenTransformers);

    public void RegisterNode<T>(bool overwrite = false) where T : class, INode
    {
        string name = T.NodeTraits.Name;
        if (overwrite)
        {
            _nodeTypes[name] = typeof(T);
        }
        else if (!_nodeTypes.TryAdd(name, typeof(T)))
        {
            throw new Exception($"A node type for '{name}' is already registered. Pass true for the '{nameof(overwrite)}' parameter if you meant to replace it.");
        }
    }

    public void RegisterNodeGenerator<T>(bool overwrite = false) where T : class, INodeGenerator
    {
        string name = T.GeneratorTraits.Name;
        if (overwrite)
        {
            _nodeGenerators[name] = typeof(T);
        }
        else if (!_nodeGenerators.TryAdd(name, typeof(T)))
        {
            throw new Exception($"A node generator for '{name}' is already registered. Pass true for the '{nameof(overwrite)}' parameter if you meant to replace it.");
        }
    }

    public void RegisterTokenResolver(string context, NodeTokenResolver resolver, bool overwrite = false)
    {
        if (overwrite)
        {
            _tokenResolvers[context] = resolver;
        }
        else if (!_tokenResolvers.TryAdd(context, resolver))
        {
            throw new Exception($"A token resolver for context '{context}' is already registered. Pass true for the '{nameof(overwrite)}' parameter if you meant to replace it.");
        }
    }

    public void RegisterTokenTransformer(string name, NodeTokenTransformer transformer, bool overwrite = false)
    {
        if (overwrite)
        {
            _tokenTransformers[name] = transformer;
        }
        else if (!_tokenTransformers.TryAdd(name, transformer))
        {
            throw new Exception($"A token transformer for name '{name}' is already registered. Pass true for the '{nameof(overwrite)}' parameter if you meant to replace it.");
        }
    }

    public void LoadProject(string projectPath)
    {
        if (!Path.IsPathRooted(projectPath))
        {
            projectPath = Path.GetFullPath(projectPath);
        }

        ProjectPath = projectPath;

        _projectDocument = LoadFile(projectPath);

        if (_projectDocument.Nodes.Count != 1 || _projectDocument.Nodes[0].Name != ProjectNode.NodeTraits.Name)
        {
            throw new Exception($"Project invalid. Expected a single root '{ProjectNode.NodeTraits.Name}' node in file: {projectPath}");
        }

        ResolveImports(_projectDocument);
    }

    public void ParseProject()
    {
        if (_projectDocument is null)
        {
            throw new Exception("Project not loaded. Call LoadProject() first.");
        }

        // Parse the KDL tree into project nodes
        if (ParseNode(_projectDocument.Nodes[0], null) is not ProjectNode projectNode)
        {
            throw new Exception($"Project invalid. Expected a root '{ProjectNode.NodeTraits.Name}' node in file: {ProjectPath}");
        }
        _projectNode = projectNode;

        // Cache some nodes for quick access later & validate their uniqueness
        ProjectContext context = CreateProjectContext();
        UpdatePluginAndModuleCaches(context);

        // Resolve any deferred nodes we encountered during parse
        while (_deferredNodes.Count > 0)
        {
            // Make a copy of the deferred nodes to process, then clear the list so that
            // any new deferred nodes added during processing will be handled in the next iteration.
            List<(KdlNode RawNode, INode Scope)> nodesToProcess = [.. _deferredNodes];
            _deferredNodes.Clear();

            ResolveDeferredNodes(nodesToProcess, context);
            UpdatePluginAndModuleCaches(context);
        }

        // Validate the resolved project nodes, this will throw if there are any validation errors
        _projectNode.Validate(null);

        // Setup the project options now that the project is fully parsed and validated
        _projectOptions.Clear();
        foreach (OptionNode optionNode in GetNodes<OptionNode>(context))
        {
            _projectOptions.Add(new ProjectOption(optionNode, optionNode.OptionType switch
            {
                EOptionType.Bool => new Option<bool?>("--" + optionNode.OptionName, () => optionNode.GetDefaultBool(), optionNode.HelpText),
                EOptionType.Int => new Option<long?>("--" + optionNode.OptionName, () => optionNode.GetDefaultNumber<long>(), optionNode.HelpText),
                EOptionType.UInt => new Option<ulong?>("--" + optionNode.OptionName, () => optionNode.GetDefaultNumber<ulong>(), optionNode.HelpText),
                EOptionType.Float => new Option<double?>("--" + optionNode.OptionName, () => optionNode.GetDefaultNumber<double>(), optionNode.HelpText),
                EOptionType.String => new Option<string?>("--" + optionNode.OptionName, () => optionNode.GetDefaultString(), optionNode.HelpText),
                _ => throw new Exception($"Unknown option type: {optionNode.OptionType}")
            }));
        }
    }

    public INode? ParseNode(KdlNode rawNode, INode? scope)
    {
        INode? node = null;

        if (scope is not null && scope.Traits.ChildNodeType is not null)
        {
            node = Activator.CreateInstance(scope.Traits.ChildNodeType, rawNode, scope) as INode
                ?? throw new Exception($"Activator.CreateInstance failed to allocate node instance for type '{scope.Traits.ChildNodeType}'.");
        }
        else if (rawNode.Name.StartsWith(':') || rawNode.Name.StartsWith('+'))
        {
            if (scope is null)
            {
                throw new NodeParseException(rawNode, "Generator and extension nodes cannot be used at the root.");
            }

            _deferredNodes.Add((rawNode, scope));
        }
        else
        {
            if (!_nodeTypes.TryGetValue(rawNode.Name, out Type? nodeType))
            {
                throw new NodeParseException(rawNode, $"'{rawNode.Name}' is not a recognized node type.");
            }

            node = Activator.CreateInstance(nodeType, rawNode, scope) as INode
                ?? throw new Exception($"Activator.CreateInstance failed to allocate node instance for type '{nodeType}'.");

            foreach (KdlNode rawChild in rawNode.Children)
            {
                if (ParseNode(rawChild, node) is INode child)
                {
                    node.Children.Add(child);
                }
            }
        }

        return node;
    }

    public ProjectContext CreateProjectContext(
        InvocationContext? invocationContext = null,
        ModuleNode? module = null,
        ConfigurationNode? configuration = null,
        PlatformNode? platform = null)
    {
        ProjectContext context = new(this)
        {
            Module = module,
            Configuration = configuration,
            Platform = platform,
            Host = PlatformNodeTraits.GetHostPlatform(),
        };

        // Search for the plugin that owns the module, if one was specified.
        INode? search = module;
        while (search is not null)
        {
            if (search is PluginNode plugin)
            {
                context.Plugin = plugin;
                break;
            }

            search = search.Scope;
        }

        // Setup project options if we have an invocation context.
        if (invocationContext is not null)
        {
            foreach (ProjectOption projectOption in _projectOptions)
            {
                object? value = invocationContext.ParseResult.GetValueForOption(projectOption.Option);
                if (value is null && projectOption.Node.EnvVarName is not null)
                {
                    value = Environment.GetEnvironmentVariable(projectOption.Node.EnvVarName);
                }

                if (value is not null)
                {
                    context.Options.Add(projectOption.Node.OptionName, value);
                }
            }
        }

        // This is done in a loop because tags may be activated by other tags. That is, it is valid
        // to write: `tags { a } when tags=a { tags { b } }`. In this case, the first loop will
        // activate `a` but not `b` because in the first loop iteration no tags are active.
        // Then the second loop will activate `b` because `a` was activated by the first iteration.
        // Once we encounter a loop iteration where no additional tags are activated, we are done.
        bool tagsChanged = true;
        while (tagsChanged)
        {
            tagsChanged = false;

            TagsNode tagsNode = GetMergedNode<TagsNode>(context, module);
            foreach (TagsEntryNode entry in tagsNode.Entries)
            {
                tagsChanged |= context.Tags.Add(entry.TagName);
            }
        }

        return context;
    }

    public List<ConfigurationNode> GetDefaultConfigurations()
    {
        KdlNode debugNode = new("configuration");
        debugNode.Arguments.Add(new KdlString("Debug"));

        KdlNode releaseNode = new("configuration");
        releaseNode.Arguments.Add(new KdlString("Release"));

        return
        [
            new ConfigurationNode(debugNode, ProjectNode),
            new ConfigurationNode(releaseNode, ProjectNode),
        ];
    }

    public List<PlatformNode> GetDefaultPlatforms()
    {
        string arch = RuntimeInformation.OSArchitecture switch
        {
            Architecture.X86 => "x86",
            Architecture.X64 => "x86_64",
            Architecture.Arm => "arm",
            Architecture.Arm64 => "arm64",
            _ => throw new Exception("Unsupported architecture.")
        };

        KdlNode node = new("platform");
        node.Properties.Add("arch", new KdlString(arch));

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            string name = RuntimeInformation.OSArchitecture switch
            {
                Architecture.X86 => "Win32",
                Architecture.X64 => "Win64",
                Architecture.Arm => "WinArm32",
                Architecture.Arm64 => "WinArm64",
                _ => throw new Exception("Unsupported architecture.")
            };

            node.Arguments.Add(new KdlString(name));
            node.Properties.Add("system", new KdlString("windows"));
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            string name = RuntimeInformation.OSArchitecture switch
            {
                Architecture.X86 => "Linux32",
                Architecture.X64 => "Linux64",
                Architecture.Arm => "LinuxArm32",
                Architecture.Arm64 => "LinuxArm64",
                _ => throw new Exception("Unsupported architecture.")
            };

            node.Arguments.Add(new KdlString(name));
            node.Properties.Add("system", new KdlString("linux"));
        }
        else
        {
            throw new Exception("Unsupported host platform.");
        }

        return [new PlatformNode(node, ProjectNode)];
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

    public PluginNode? TryGetPluginById(string pluginId)
    {
        if (_plugins.TryGetValue(pluginId, out PluginNode? plugin))
        {
            return plugin;
        }
        return null;
    }

    public List<ModuleDependency> GetModuleDependencies(ProjectContext context, ModuleNode module, ENodeDependencyInheritance inheritance)
    {
        List<ModuleDependency> result = [];
        Dictionary<DependenciesEntryNode, int> indexMap = [];

        GetModuleDependenciesInternal(context, module, result, indexMap, inheritance, false);

        return result;
    }

    public List<T> GetNodes<T>(ProjectContext context, INode? scope = null, bool searchDependencies = true) where T : class, INode
    {
        return GetNodes<T>(context, scope, (v) => true, searchDependencies);
    }

    public List<T> GetNodes<T>(ProjectContext context, INode? scope, Func<T, bool> filter, bool searchDependencies = true) where T : class, INode
    {
        List<T> resolvedNodes = [];

        foreach (T node in EnumerateNodes(context, scope, filter, searchDependencies).Reverse())
        {
            T resolved = Activator.CreateInstance(typeof(T), new KdlNode(T.NodeTraits.Name), scope) as T
                ?? throw new Exception($"Activator.CreateInstance failed to allocate node instance for type '{typeof(T)}'.");
            resolved.MergeAndResolve(context, node);
            resolvedNodes.Add(resolved);
        }

        return resolvedNodes;
    }

    public T GetMergedNode<T>(ProjectContext context, INode? scope = null, bool searchDependencies = true) where T : class, INode
    {
        return GetMergedNode<T>(context, scope, (v) => true, searchDependencies);
    }

    public T GetMergedNode<T>(ProjectContext context, INode? scope, Func<T, bool> filter, bool searchDependencies = true) where T : class, INode
    {
        T resolved = Activator.CreateInstance(typeof(T), new KdlNode(T.NodeTraits.Name), scope) as T
            ?? throw new Exception($"Activator.CreateInstance failed to allocate node instance for type '{typeof(T)}'.");

        foreach (T node in EnumerateNodes(context, scope, filter, searchDependencies).Reverse())
        {
            resolved.MergeAndResolve(context, node);
        }

        return resolved;
    }

    private void UpdatePluginAndModuleCaches(ProjectContext context)
    {
        _plugins.Clear();
        foreach (PluginNode plugin in EnumerateNodes<PluginNode>(context, null, false))
        {
            if (!_plugins.TryAdd(plugin.PluginName, plugin))
            {
                throw new NodeValidationException(plugin, $"Encountered duplicate plugin ID '{plugin.PluginName}'. Plugin IDs must be unique. Previously defined in: {_plugins[plugin.PluginName].Node.SourceInfo.ToErrorString()}");
            }
        }

        _modules.Clear();
        foreach (ModuleNode module in EnumerateNodes<ModuleNode>(context, null, false))
        {
            if (!_modules.TryAdd(module.ModuleName, module))
            {
                throw new NodeValidationException(module, $"Encountered duplicate module name '{module.ModuleName}'. Module names must be unique. Previously defined in: {_modules[module.ModuleName].Node.SourceInfo.ToErrorString()}");
            }
        }
    }

    private IEnumerable<T> EnumerateNodes<T>(ProjectContext context, INode? scope, bool searchDependencies) where T : class, INode
    {
        return EnumerateNodes<T>(context, scope, (v) => true, searchDependencies);
    }

    private IEnumerable<T> EnumerateNodes<T>(ProjectContext context, INode? scope, Func<T, bool> filter, bool searchDependencies) where T : class, INode
    {
        scope ??= ProjectNode;

        INode? originalScope = scope;

        // Search the scope chain for the requested nodes, both downward and upward.
        Stack<INode> stack = new();
        while (scope is not null)
        {
            stack.Clear();
            stack.Push(scope);

            // Search the scope children, descending into any when nodes we encounter
            while (stack.Count > 0)
            {
                INode s = stack.Pop();
                foreach (INode child in s.Children)
                {
                    if (child is T instance && filter(instance))
                    {
                        yield return instance;
                    }
                    else if (child is WhenNode whenNode)
                    {
                        if (whenNode.IsActive(context))
                        {
                            stack.Push(whenNode);
                        }
                    }
                }
            }

            // Walk up the scope chain, but only check scopes where the node is allowed
            do
            {
                scope = scope.Scope;
            } while (scope is not null && !T.NodeTraits.ValidScopes.Contains(scope.Node.Name));
        }

        // Optionally search depdenencies for the requested nodes, but only if the node is
        // allowed to be inherited through dependencies.
        ENodeDependencyInheritance inheritance = T.NodeTraits.DependencyInheritance;
        if (searchDependencies
            && inheritance != ENodeDependencyInheritance.None
            && T.NodeTraits.ValidScopes.Contains(PublicNode.NodeTraits.Name)
            && originalScope is ModuleNode module)
        {
            List<ModuleDependency> dependencies = GetModuleDependencies(context, module, inheritance);
            foreach (ModuleDependency dep in dependencies)
            {
                if (dep.Module is ModuleNode dependencyModule)
                {
                    foreach (T node in EnumerateNodes(context, dependencyModule, filter, true))
                    {
                        if (IsWithinPublicNode(node))
                        {
                            yield return node;
                        }
                    }
                }
            }
        }
    }

    private KdlDocument LoadFile(string path)
    {
        if (!Path.IsPathRooted(path))
        {
            throw new ArgumentException($"Path must be absolute: {path}", nameof(path));
        }

        if (_kdlFiles.TryGetValue(path, out KdlDocument? document))
        {
            return document;
        }

        KdlDocument doc = KdlDocument.FromFile(path);
        _kdlFiles[path] = doc;

        return doc;
    }

    private void ResolveDeferredNodes(List<(KdlNode RawNode, INode Scope)> nodesToProcess, ProjectContext context)
    {
        foreach ((KdlNode rawNode, INode scope) in nodesToProcess)
        {
            // Extension node
            if (rawNode.Name.StartsWith('+'))
            {
                ResolveExtensionNode(context, rawNode);
                rawNode.RemoveFromParent();
            }
            // Generator node
            else if (rawNode.Name.StartsWith(':'))
            {
                ResolveGeneratorNode(context, rawNode, scope);
                rawNode.RemoveFromParent();
            }
            // Unknown deferred node type (should never happen)
            else
            {
                throw new NodeParseException(rawNode, "Deferred node is neither an extension nor a generator.");
            }
        }
    }

    private void ResolveExtensionNode(ProjectContext context, KdlNode extensionNode)
    {
        if (extensionNode.Arguments.Count == 0 || extensionNode.Arguments[0] is not KdlString s || string.IsNullOrEmpty(s.Value))
        {
            throw new NodeParseException(extensionNode, "Expected string argument in extension node for the module name or plugin ID.");
        }

        string nodeId = s.Value;
        string nodeName = extensionNode.Name[1..];
        if (nodeName == PluginNode.NodeTraits.Name)
        {
            if (_plugins.TryGetValue(nodeId, out PluginNode? plugin))
            {
                CopyToNode(plugin, extensionNode);
            }
            else if (extensionNode.Properties.TryGetValue("required", out KdlValue? requiredValue)
                && requiredValue is KdlBool requiredBool
                && requiredBool.Value)
            {
                throw new NodeParseException(extensionNode, $"Failed to resolve required plugin '{nodeId}'.");
            }
        }
        else if (nodeName == ModuleNode.NodeTraits.Name)
        {
            if (_modules.TryGetValue(nodeId, out ModuleNode? module))
            {
                CopyToNode(module, extensionNode);
            }
            else if (extensionNode.Properties.TryGetValue("required", out KdlValue? requiredValue)
                && requiredValue is KdlBool requiredBool
                && requiredBool.Value)
            {
                throw new NodeParseException(extensionNode, $"Failed to resolve required module '{nodeId}'.");
            }
        }
        else
        {
            throw new NodeParseException(extensionNode, $"Unknown extension node type '{nodeName}'. Expected '{PluginNode.NodeTraits.Name}' or '{ModuleNode.NodeTraits.Name}'.");
        }
    }

    private void ResolveGeneratorNode(ProjectContext context, KdlNode generatorNode, INode scope)
    {
        string nodeName = generatorNode.Name[1..];
        if (!_nodeGenerators.TryGetValue(nodeName, out Type? generatorType))
        {
            throw new NodeParseException(generatorNode, $"Unknown generator node type '{nodeName}'.");
        }

        INodeGenerator generator = Activator.CreateInstance(generatorType, context) as INodeGenerator
            ?? throw new NodeParseException(generatorNode, $"Activator.CreateInstance failed to allocate node generator instance for type '{generatorType}'.");

        generator.GenerateNodes(generatorNode, scope);
    }

    private void CopyToNode(INode target, KdlNode source)
    {
        source.CopyTo(target.Node);

        foreach (KdlNode rawChild in target.Node.Children)
        {
            if (ParseNode(rawChild, target) is not INode child)
            {
                throw new NodeParseException(rawChild, $"Failed to parse child node '{rawChild.Name}' from node '{source.Name}'.");
            }
            target.Children.Add(child);
        }
    }

    private void ResolveImports(KdlDocument document)
    {
        List<KdlNode> importNodes = [.. document.GetNodesByName("import")];
        ResolveImports(importNodes);
    }

    private void ResolveImports(List<KdlNode> importNodes)
    {
        foreach (KdlNode importNode in importNodes)
        {
            if (importNode.Parent is null || !ImportNode.NodeTraits.ValidScopes.Contains(importNode.Parent.Name))
            {
                throw new NodeParseException(importNode, $"Invalid parent scope. Expected one of: {string.Join(", ", ImportNode.NodeTraits.ValidScopes)}.");
            }

            if (importNode.Arguments.Count != 1 || importNode.Arguments[0] is not KdlString kdlImportPath)
            {
                throw new NodeParseException(importNode, "Expected exactly one string argument for the import path.");
            }

            string importPath = kdlImportPath.Value;

            if (!Path.GetExtension(importPath).Equals(".kdl", StringComparison.OrdinalIgnoreCase))
            {
                importPath = Path.Combine(importPath, "he_plugin.kdl");
            }

            string directory = Path.GetDirectoryName(importNode.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();
            Matcher matcher = new();
            matcher.AddInclude(importPath);
            foreach (string importMatchedPath in matcher.GetResultsInFullPath(directory))
            {
                KdlDocument importedDoc = LoadFile(importMatchedPath);
                importNode.ReplaceInParent(importedDoc.Nodes);
                ResolveImports(importedDoc);
            }
        }
    }

    private static bool IsWithinPublicNode(INode node)
    {
        INode? scope = node.Scope;

        while (scope is not null)
        {
            if (scope is PublicNode)
            {
                return true;
            }

            scope = scope.Scope;
        }

        return false;
    }

    private void GetModuleDependenciesInternal(
        ProjectContext context,
        ModuleNode module,
        List<ModuleDependency> result,
        Dictionary<DependenciesEntryNode, int> indexMap,
        ENodeDependencyInheritance inheritance,
        bool publicOnly)
    {
        List<ModuleNode> modulesToRecurse = [];

        // First add all our immediate dependencies to the list
        foreach (DependenciesNode dependencies in EnumerateNodes<DependenciesNode>(context, module, false))
        {
            if (publicOnly && !IsWithinPublicNode(dependencies))
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
            GetModuleDependenciesInternal(context, dependencyModule, result, indexMap, inheritance, true);
        }
    }
}
