// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;
using System.CommandLine;
using System.CommandLine.Invocation;

namespace Harvest.Make.Projects;

public class ProjectOption(OptionNode node, Option option)
{
    public OptionNode Node => node;
    public Option Option => option;
}

public class ModuleDependency(DependenciesEntryNode entry, ModuleNode? resolvedModule)
{
    public string DependencyName => entry.DependencyName;
    public EDependencyKind Kind => entry.Kind;
    public bool IsExternal { get; set; } = entry.IsExternal;
    public bool IsWholeArchive { get; set; } = entry.IsWholeArchive;

    public ModuleNode? Module => resolvedModule;
}

public interface IProjectService
{
    public string ProjectPath { get; }
    public KdlDocument ProjectDocument { get; }
    public ProjectNode ProjectNode { get; }
    public IReadOnlyList<ProjectOption> ProjectOptions { get; }
    public NodeTokenReplacer TokenReplacer { get; }

    public void RegisterNode<T>(bool overwrite = false) where T : class, INode;
    public void RegisterNodeGenerator<T>(bool overwrite = false) where T : class, INodeGenerator;
    public void RegisterTokenResolver(string context, NodeTokenResolver resolver, bool overwrite = false);
    public void RegisterTokenTransformer(string name, NodeTokenTransformer transformer, bool overwrite = false);

    public void LoadProject(string projectPath);
    public void ParseProject();
    public INode? ParseNode(KdlNode rawNode, INode? scope);

    public ProjectContext CreateProjectContext(
        InvocationContext? invocationContext = null,
        ModuleNode? module = null,
        ConfigurationNode? configuration = null,
        PlatformNode? platform = null);

    public List<ConfigurationNode> GetDefaultConfigurations();
    public List<PlatformNode> GetDefaultPlatforms();

    public IEnumerable<ModuleNode> GetAllModules();
    public ModuleNode? TryGetModuleByName(string moduleName);

    public IEnumerable<PluginNode> GetAllPlugins();
    public PluginNode? TryGetPluginById(string pluginId);

    public List<ModuleDependency> GetModuleDependencies(ProjectContext context, ModuleNode module, ENodeDependencyInheritance inheritance);

    public List<T> GetNodes<T>(ProjectContext context, INode? scope = null, bool searchDepdencies = true) where T : class, INode;
    public List<T> GetNodes<T>(ProjectContext context, INode? scope, Func<T, bool> filter, bool searchDepdencies = true) where T : class, INode;

    public T GetMergedNode<T>(ProjectContext context, INode? scope = null, bool searchDepdencies = true) where T : class, INode;
    public T GetMergedNode<T>(ProjectContext context, INode? scope, Func<T, bool> filter, bool searchDepdencies = true) where T : class, INode;
}
