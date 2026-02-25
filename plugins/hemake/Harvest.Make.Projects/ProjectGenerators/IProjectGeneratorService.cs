// Copyright Chad Engler

namespace Harvest.Make.Projects.ProjectGenerators;

public interface IProjectGeneratorService
{
    public Task GenerateProjectFilesAsync();
}
