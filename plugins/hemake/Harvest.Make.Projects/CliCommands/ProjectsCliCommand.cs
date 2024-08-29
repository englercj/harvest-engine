// Copyright Chad Engler

using DotMake.CommandLine;
using Harvest.Make.CliCommands;
using Harvest.Make.Projects.Generators;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;

namespace Harvest.Make.Projects.CliCommands;

[CliCommand(Description = "Generates IDE project files.")]
public class ProjectsCliCommand
{
    public abstract class BaseProjectGeneratorCliCommand(
        ILogger logger,
        IProjectGeneratorService generatorService,
        IProjectService projectService) : ICliCommand
    {
        protected readonly ILogger _logger = logger;
        protected readonly IProjectService _projectService = projectService;
        protected readonly IProjectGeneratorService _generatorService = generatorService;

        [CliOption(Name = "--project", Description = "The Harvest Engine project file for your project.", Required = true)]
        public FileInfo? ProjectFile { get; set; }

        public async Task<int> RunCommandAsync()
        {
            GenerateProjectResult result = await _generatorService.GenerateProjectFilesAsync();
            if (!result.Success)
            {
                _logger.LogError(result.ErrorMessage);
                return -1;
            }

            return 0;
        }

        public void ValidateCommand()
        {
            if (ProjectFile is null)
            {
                throw new Exception("Project path not specified, or doesn't exist (--project)");
            }
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
