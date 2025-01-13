// Copyright Chad Engler

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
    public bool WholeArchive { get; set; } = entry.WholeArchive;

    public ModuleNode? Module => resolvedModule;
}

public interface IProjectService
{
    public string ProjectPath { get; }
    public List<ProjectOption> Options { get; }

    public void RegisterNodeType<T>() where T : class, INode;
    public void LoadProject(string projectPath);

    public ProjectContext GetProjectContext(InvocationContext? invocationContext = null, ModuleNode? module = null, ConfigurationNode? configuration = null, PlatformNode? platform = null);
    public List<ConfigurationNode> GetDefaultConfigurations();
    public List<PlatformNode> GetDefaultPlatforms();

    public ModuleNode? TryGetModule(string moduleName);

    public List<ModuleDependency> GetModuleDependencies(ProjectContext context, ModuleNode module, ENodeDependencyInheritance inheritance);

    public IEnumerable<T> FindNodes<T>(ProjectContext context, INode? scope = null, bool searchDepdencies = true) where T : class, INode;
    public T GetResolvedNode<T>(ProjectContext context, INode? scope = null) where T : class, INode;
    public T GetResolvedNode<T>(ProjectContext context, INode? scope, Func<T, bool> filter) where T : class, INode;
}
