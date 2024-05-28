// Copyright Chad Engler

namespace Harvest.Make.Projects.Generators;

public class GenerateProjectResult
{
    public bool Success { get; set; }
    public string? ErrorMessage { get; set; }
}

public interface IProjectGeneratorService
{
    public Task<GenerateProjectResult> GenerateProjectFilesAsync(IProjectContext ctx, IProjectStructure data);
}
