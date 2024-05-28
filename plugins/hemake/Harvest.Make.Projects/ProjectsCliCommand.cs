// Copyright Chad Engler

using DotMake.CommandLine;
using Harvest.Make.CliCommands;
using Harvest.Make.Projects.Generators;
using Harvest.Make.Services;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;

namespace Harvest.Make.Commands;

[CliCommand(
    Description = "Generates IDE project files.",
    Parent = typeof(RootCliCommand)
)]
public class ProjectsCliCommand
{
    public class BaseProjectGeneratorCliCommand : BaseProjectCliCommand
    {
        protected readonly ILogger _logger;
        protected readonly IProjectGeneratorService _generatorService;

        public BaseProjectGeneratorCliCommand(
            ILogger logger,
            IProjectGeneratorService generatorService,
            IProjectService projectService)
            : base(projectService)
        {
            _logger = logger;
            _generatorService = generatorService;
        }

        public async Task<int> RunAsync()
        {
            if (ProjectService.ProjectPath is null)
            {
                _logger.LogError("No project file specified.");
                return 1;
            }

            await _generatorService.GenerateProjectFilesAsync();
            return 0;
        }
    }

    [CliCommand(Name = ProjectGeneratorNames.VS2022, Description = "Generate Visual Studio 2022 project files.")]
    public class VS2022CliCommand : BaseProjectGeneratorCliCommand
    {
        public VS2022CliCommand(
            ILogger<VS2022CliCommand> logger,
            [FromKeyedServices(ProjectGeneratorNames.VS2022)] IProjectGeneratorService generatorService,
            IProjectService projectService)
            : base(logger, generatorService, projectService)
        { }
    }

    [CliCommand(Name = ProjectGeneratorNames.GNUMake, Description = "Generate GNU Make makefiles.")]
    public class GNUMakeCliCommand : BaseProjectGeneratorCliCommand
    {
        public GNUMakeCliCommand(
            ILogger<GNUMakeCliCommand> logger,
            [FromKeyedServices(ProjectGeneratorNames.GNUMake)] IProjectGeneratorService generatorService,
            IProjectService projectService)
            : base(logger, generatorService, projectService)
        { }
    }
}
