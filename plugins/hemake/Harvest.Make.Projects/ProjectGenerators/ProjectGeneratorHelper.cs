// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using System.CommandLine.Invocation;

namespace Harvest.Make.Projects.ProjectGenerators;

public class ProjectGeneratorHelper
{
    private readonly IProjectService _projectService;

    public InvocationContext CliContext { get; }
    public ProjectContext BaseContext { get; }

    public ProjectNode Project { get; }
    public BuildOutputNode BuildOutput { get; }
    public List<ConfigurationNode> Configurations { get; }
    public List<PlatformNode> Platforms { get; }

    public ProjectGeneratorHelper(IProjectService projectService, InvocationContext context)
    {
        _projectService = projectService;
        CliContext = context;

        BaseContext = _projectService.CreateProjectContext(context);
        Project = _projectService.GetMergedNode<ProjectNode>(BaseContext);
        BuildOutput = _projectService.GetMergedNode<BuildOutputNode>(BaseContext);
        Configurations = _projectService.GetNodes<ConfigurationNode>(BaseContext);
        Platforms = _projectService.GetNodes<PlatformNode>(BaseContext);

        if (Configurations.Count == 0)
        {
            Configurations = _projectService.GetDefaultConfigurations();
        }

        if (Platforms.Count == 0)
        {
            Platforms = _projectService.GetDefaultPlatforms();
        }
    }

    public void ForEachConfig(Action<ConfigurationNode, PlatformNode> action)
    {
        foreach (ConfigurationNode configuration in Configurations)
        {
            foreach (PlatformNode platform in Platforms)
            {
                action(configuration, platform);
            }
        }
    }
}
