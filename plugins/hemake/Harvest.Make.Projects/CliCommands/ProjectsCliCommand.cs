// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.CliCommands;
using Harvest.Make.Projects.ProjectGenerators;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using System.CommandLine.Invocation;

namespace Harvest.Make.Projects.CliCommands;

public abstract class BaseProjectGeneratorCliCommand(
    ILogger<BaseProjectGeneratorCliCommand> logger,
    IProjectGeneratorService generatorService,
    IProjectService projectService)
    : ICliCommand
{
    protected readonly ILogger _logger = logger;
    protected readonly IProjectService _projectService = projectService;
    protected readonly IProjectGeneratorService _generatorService = generatorService;

    public abstract string Name { get; }
    public abstract string Description { get; }

    public async Task<int> RunCommandAsync(InvocationContext context)
    {
        await _generatorService.GenerateProjectFilesAsync(context);
        return 0;
    }
}

[Service<ICliCommand>(Enumerable = true)]
public class VS2022CliCommand(
    ILogger<VS2022CliCommand> logger,
    [FromKeyedServices(ProjectGeneratorNames.VS2022)] IProjectGeneratorService generatorService,
    IProjectService projectService)
    : BaseProjectGeneratorCliCommand(logger, generatorService, projectService)
{
    public override string Name => ProjectGeneratorNames.VS2022;
    public override string Description => "Generate Visual Studio 2022 project files.";
}
