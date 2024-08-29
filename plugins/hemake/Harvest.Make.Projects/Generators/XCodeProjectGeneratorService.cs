// Copyright Chad Engler

using Harvest.Make.Attributes;

namespace Harvest.Make.Projects.Generators;

[Service<IProjectGeneratorService>(Key = ProjectGeneratorNames.XCode)]
public class XCodeProjectGeneratorService : IProjectGeneratorService
{
    private IProjectService _projectService;

    XCodeProjectGeneratorService(IProjectService projectService)
    {
        _projectService = projectService;
    }

    public void GenerateProjectFiles()
    {

    }
}
