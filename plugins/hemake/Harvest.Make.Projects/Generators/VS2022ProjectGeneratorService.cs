// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.Services;
using System.Text;

namespace Harvest.Make.Projects.Generators;

[Service<IProjectGeneratorService>(Key = ProjectGeneratorNames.VS2022)]
public class VS2022ProjectGeneratorService : IProjectGeneratorService
{
    private IProjectService _projectService;
    private StringBuilder _buf = new(4096);

    VS2022ProjectGeneratorService(IProjectService projectService)
    {
        _projectService = projectService;
    }

    public void GenerateProjectFiles()
    {

    }

    private void 
}
