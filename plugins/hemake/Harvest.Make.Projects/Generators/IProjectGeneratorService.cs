// Copyright Chad Engler

using System.CommandLine.Invocation;

namespace Harvest.Make.Projects.Generators;

public interface IProjectGeneratorService
{
    public void GenerateProjectFiles(InvocationContext context);
}
