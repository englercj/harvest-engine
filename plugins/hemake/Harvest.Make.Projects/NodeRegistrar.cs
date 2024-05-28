// Copyright Chad Engler

using DotMake.CommandLine;
using Harvest.Make.Attributes;
using Harvest.Make.Commands;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace Harvest.Make.Projects;

[Service<IHostedService>(Enumerable = true)]
public class NodeRegistrar : IHostedService
{
    NodeRegistrar(IProjectService projectService)
    {
        projectService.LoadProjectAsync("Makefile");
    }

    public Task StartAsync(CancellationToken cancellationToken)
    {
        RegisterNode<ModuleNode>();
        throw new NotImplementedException();
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        throw new NotImplementedException();
    }
}
