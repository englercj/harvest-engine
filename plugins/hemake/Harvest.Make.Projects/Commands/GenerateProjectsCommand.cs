// Copyright Chad Engler

using Harvest.Common.Attributes;
using Harvest.Make.Projects.ProjectGenerators;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using System.CommandLine.Invocation;

namespace Harvest.Make.Projects.Commands;

[Command("Generate IDE / build system project files.")]
internal partial class GenerateProjectsCommand(
    ILogger<GenerateProjectsCommand> logger,
    IEnumerable<IProjectGeneratorService> projectGeneratorServices)
    : ICliCommand
{
    [Argument("The source directory to upload.", Arity = ArgArity.ExactlyOne)]
    public string Name { get; init; } = "";

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
