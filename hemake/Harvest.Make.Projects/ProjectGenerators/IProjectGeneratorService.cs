// Copyright Chad Engler

namespace Harvest.Make.Projects.ProjectGenerators;

public interface IProjectGeneratorService
{
    public string Name { get; }

    public Task GenerateProjectFilesAsync();
}
