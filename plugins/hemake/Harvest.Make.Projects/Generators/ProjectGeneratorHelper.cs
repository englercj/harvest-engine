// Copyright Chad Engler

using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.Configuration;
using System.CommandLine.Invocation;

namespace Harvest.Make.Projects.Generators;

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

        BaseContext = _projectService.GetProjectContext(context);
        Project = _projectService.GetResolvedNode<ProjectNode>(BaseContext);
        BuildOutput = _projectService.GetResolvedNode<BuildOutputNode>(BaseContext);
        Configurations = _projectService.FindNodes<ConfigurationNode>(BaseContext).ToList();
        Platforms = _projectService.FindNodes<PlatformNode>(BaseContext).ToList();

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
