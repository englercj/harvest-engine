// Copyright Chad Engler

using DotMake.CommandLine;
using Harvest.Make.Services;

namespace Harvest.Make.CliCommands;

public class BaseProjectCliCommand
{
    private readonly IProjectService _projectService;

    public IProjectService ProjectService => _projectService;

    [CliOption(Name = "--project", Description = "The Harvest Engine project file for your project.", Required = true)]
    public FileInfo? ProjectPath { get; set; }

    public BaseProjectCliCommand(IProjectService projectService)
    {
        _projectService = projectService;
    }
}
