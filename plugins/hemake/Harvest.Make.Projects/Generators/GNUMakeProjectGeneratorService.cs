// Copyright Chad Engler

using Harvest.Make.Attributes;

namespace Harvest.Make.Projects.Generators;

[Service<IProjectGeneratorService>(Key = ProjectGeneratorNames.GNUMake)]
public class GNUMakeProjectGeneratorService : IProjectGeneratorService
{
    public void GenerateProjectFiles()
    {
        throw new NotImplementedException();
    }
}
