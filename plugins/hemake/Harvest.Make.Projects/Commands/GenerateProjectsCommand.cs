// Copyright Chad Engler

using Harvest.Common;
using Harvest.Common.Attributes;
using Harvest.Make.Projects.ProjectGenerators;
using Microsoft.Extensions.Logging;

namespace Harvest.Make.Projects.Commands;

[Command("Generate IDE / build system project files.")]
internal partial class GenerateProjectsCommand(
    ILogger<GenerateProjectsCommand> logger,
    IEnumerable<IProjectGeneratorService> projectGeneratorServices)
    : ICommandExecutor
{
    [Argument("The name of the project generator to use.", Arity = ArgArity.ExactlyOne)]
    public string Name { get; set; } = "";

    public async Task<int> ExecuteAsync(CancellationToken ct)
    {
        foreach (IProjectGeneratorService generatorService in projectGeneratorServices)
        {
            if (generatorService.Name == Name)
            {
                logger.LogInformation("Generating project files with generator: {GeneratorName}", generatorService.Name);
                try
                {
                    await generatorService.GenerateProjectFilesAsync();
                    logger.LogInformation("Project files generated successfully.");
                    return 0;
                }
                catch (Exception ex)
                {
                    logger.LogError(ex, "An error occurred while generating project files with generator: {GeneratorName}", generatorService.Name);
                    return 1;
                }
            }
        }

        logger.LogError("No project generator found for generator name: {GeneratorName}", Name);
        return 1;
    }
}
