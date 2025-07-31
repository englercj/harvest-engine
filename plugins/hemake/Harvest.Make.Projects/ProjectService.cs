// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Exceptions;
using Harvest.Kdl.Types;
using Harvest.Make.Attributes;
using Harvest.Make.Extensions;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.FileSystemGlobbing;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.IO;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects;

[Service<IProjectService>]
public class ProjectService : IProjectService
{
    private readonly Dictionary<string, Type> _nodeTypes = [];
    private readonly Dictionary<string, KdlDocument> _kdlFiles = [];
    private readonly Dictionary<string, ModuleNode> _modules = [];
    private readonly List<ProjectOption> _projectOptions = [];
    private KdlDocument? _projectDocument;
    private ProjectNode? _projectNode;

    public string ProjectPath { get; private set; } = string.Empty;
    public KdlDocument ProjectDocument => _projectDocument ?? throw new Exception("Project not loaded. Call LoadProject() first.");
    public ProjectNode ProjectNode => _projectNode ?? throw new Exception("Project not parsed. Call ParseProject() first.");
    public IReadOnlyList<ProjectOption> ProjectOptions => _projectOptions;

    public void RegisterNodeType<T>() where T : class, INode
    {
        _nodeTypes.Add(NodeTraits<T>.Name, typeof(T));
    }

    public void LoadProject(string projectPath)
    {
        if (!Path.IsPathRooted(projectPath))
        {
            projectPath = Path.GetFullPath(projectPath);
        }

        ProjectPath = projectPath;

        _projectDocument = LoadFile(projectPath);

        if (_projectDocument.Nodes.Count != 1 || _projectDocument.Nodes[0].Name != ProjectNode.NodeName)
        {
            throw new Exception($"Project invalid. Expected a single root '{ProjectNode.NodeName}' node in file: {projectPath}");
        }

        ResolveImports(_projectDocument);
    }

    public void ParseProject()
    {
        if (_projectDocument is null)
        {
            throw new Exception("Project not loaded. Call LoadProject() first.");
        }

        _projectNode = (ProjectNode)CreateAndValidateNode(ProjectPath, _projectDocument.Nodes[0], null);

        ProjectContext context = CreateProjectContext();
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

        IEnumerable<ModuleNode> modules = GetNodes<ModuleNode>(context);
        foreach (ModuleNode module in modules)
        {
            _modules.Add(module.ModuleName, module);
        }
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
            Host = PlatformNode.GetHostPlatform(),
        };

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
                tagsChanged |= context.Tags.Add(entry.Tag);
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

    public ModuleNode? TryGetModule(string moduleName)
    {
        if (_modules.TryGetValue(moduleName, out ModuleNode? module))
        {
            return module;
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

    public List<T> GetNodes<T>(ProjectContext context, INode? scope = null, bool searchDepdencies = true) where T : class, INode
    {
        return GetNodes<T>(context, scope, (v) => true, searchDepdencies);
    }

    public List<T> GetNodes<T>(ProjectContext context, INode? scope, Func<T, bool> filter, bool searchDepdencies = true) where T : class, INode
    {
        List<T> resolvedNodes = [];

        foreach (T node in EnumerateNodes(context, scope, filter, searchDepdencies).Reverse())
        {
            T resolved = NodeTraits<T>.CreateInstance(scope);
            resolved.MergeAndResolve(context, node);
            resolvedNodes.Add(resolved);
        }

        return resolvedNodes;
    }

    public T GetMergedNode<T>(ProjectContext context, INode? scope = null, bool searchDepdencies = true) where T : class, INode
    {
        return GetMergedNode<T>(context, scope, (v) => true, searchDepdencies);
    }

    public T GetMergedNode<T>(ProjectContext context, INode? scope, Func<T, bool> filter, bool searchDepdencies = true) where T : class, INode
    {
        T resolved = NodeTraits<T>.CreateInstance(scope);

        foreach (T node in EnumerateNodes(context, scope, filter, searchDepdencies).Reverse())
        {
            resolved.MergeAndResolve(context, node);
        }

        return resolved;
    }

    private IEnumerable<T> EnumerateNodes<T>(ProjectContext context, INode? scope, bool searchDepdencies) where T : class, INode
    {
        return EnumerateNodes<T>(context, scope, (v) => true, searchDepdencies);
    }

    private IEnumerable<T> EnumerateNodes<T>(ProjectContext context, INode? scope, Func<T, bool> filter, bool searchDepdencies) where T : class, INode
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
                    else if (child.Name == WhenNode.NodeName)
                    {
                        WhenNode whenNode = (WhenNode)child;
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
            } while (scope is not null && !NodeTraits<T>.Scopes.Contains(scope.Name));
        }

        // Optionally search depdenencies for the requested nodes, but only if the node is
        // allowed to be inherited through dependencies.
        ENodeDependencyInheritance inheritance = NodeTraits<T>.DependencyInheritance;
        if (searchDepdencies
            && inheritance != ENodeDependencyInheritance.None
            && NodeTraits<T>.Scopes.Contains(PublicNode.NodeName)
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

    private INode CreateAndValidateNode(string filePath, KdlNode rawNode, INode? scope)
    {
        INode? node;

        if (scope is not null && scope.ChildNodeType is not null)
        {
            if (Activator.CreateInstance(scope.ChildNodeType, rawNode, scope) is not INode instance)
            {
                throw new Exception($"Project invalid. Failed to create node instance for '{rawNode.Name}' in file: {filePath}");
            }

            node = instance;
        }
        else
        {
            // Remove the plus from extension nodes so we can find the type by name.
            string nodeName = rawNode.Name;
            if (nodeName.StartsWith('+'))
            {
                nodeName = nodeName[1..];
            }

            // TODO: generators (':'), and extensions ('+')

            if (!_nodeTypes.TryGetValue(nodeName, out Type? nodeType))
            {
                throw new Exception($"Project invalid. Unknown node '{nodeName}' in file: {filePath}");
            }

            if (Activator.CreateInstance(nodeType, rawNode, scope) is not INode instance)
            {
                throw new Exception($"Project invalid. Failed to create node instance for '{nodeName}' in file: {filePath}");
            }

            node = instance;
            foreach (KdlNode rawChild in rawNode.Children)
            {
                INode child = CreateAndValidateNode(filePath, rawChild, node);
                node.Children.Add(child);
            }
        }

        NodeValidationResult result = node.Validate(scope);
        if (!result.IsValid)
        {
            throw new Exception($"Project invalid. Validation failed for '{node.Name}' node.\n{node.Node.SourceInfo.ToErrorString()}: {result.ErrorContent}");
        }

        return node;
    }

    private void ResolveImports(KdlDocument document)
    {
        List<KdlNode> importNodes = [.. document.GetNodesByName("import")];
        ResolveImports(document.SourceInfo.FilePath, importNodes);
    }

    private void ResolveImports(string filePath, List<KdlNode> importNodes)
    {
        foreach (KdlNode importNode in importNodes)
        {
            if (importNode.Parent is null || !ImportNode.NodeScopes.Contains(importNode.Parent.Name))
            {
                throw new KdlException($"Invalid project. An 'import' node cannot be used in this scope.", importNode.SourceInfo);
            }

            if (importNode.Arguments.Count != 1 || importNode.Arguments[0] is not KdlString kdlImportPath)
            {
                throw new KdlException($"Invalid project. An 'import' must have exactly one string argument.", importNode.SourceInfo);
            }

            string importPath = kdlImportPath.Value;

            if (!Path.GetExtension(importPath).Equals(".kdl", StringComparison.OrdinalIgnoreCase))
            {
                importPath = Path.Combine(importPath, "he_plugin.kdl");
            }

            string directory = Path.GetDirectoryName(filePath) ?? Directory.GetCurrentDirectory();
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
            if (scope.Name == PublicNode.NodeName)
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
                            if (TryGetModule(entry.DependencyName) is ModuleNode dependencyModule)
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
                                // TODO: check to ensure depdencies existing is checked during the validation phase
                                throw new Exception($"Project invalid. Failed to resolve dependency '{entry.DependencyName}' for module '{module.ModuleName}'");
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
                                if (TryGetModule(entry.DependencyName) is ModuleNode dependencyModule)
                                {
                                    resolvedDepdencyModule = dependencyModule;
                                }
                                else
                                {
                                    // TODO: check to ensure depdencies existing is checked during the validation phase
                                    throw new Exception($"Project invalid. Failed to resolve dependency '{entry.DependencyName}' for module '{module.ModuleName}'");
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
