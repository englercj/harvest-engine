// Copyright Chad Engler

using System.CommandLine.Invocation;

namespace Harvest.Make.Projects.ProjectGenerators;

public interface IProjectGeneratorService
{
    public Task GenerateProjectFilesAsync(InvocationContext context);
}
