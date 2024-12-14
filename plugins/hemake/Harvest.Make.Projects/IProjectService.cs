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

public interface IProjectService
{
    public string ProjectPath { get; }
    public List<ProjectOption> Options { get; }

    public void RegisterNodeType<T>() where T : class, INode;
    public void LoadProject(string projectPath);

    public ProjectContext GetProjectContext(InvocationContext? invocationContext = null, ModuleNode? module = null, ConfigurationNode? configuration = null, PlatformNode? platform = null);
    public List<ConfigurationNode> GetDefaultConfigurations();
    public List<PlatformNode> GetDefaultPlatforms();

    public IEnumerable<T> FindNodes<T>(ProjectContext context, INode? scope = null) where T : class, INode;
    public T GetResolvedNode<T>(ProjectContext context, INode? scope = null) where T : class, INode;
    public T GetResolvedNode<T>(ProjectContext context, INode? scope, Func<T, bool> filter) where T : class, INode;
}
