// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.Hosting;

namespace Harvest.Make.Projects;

[Service<IHostedService>(Enumerable = true)]
public class NodeRegistrar(IProjectService projectService) : IHostedService
{
    private readonly IProjectService _projectService = projectService;

    public Task StartAsync(CancellationToken cancellationToken)
    {
        _projectService.RegisterNodeType<ArchiveNode>();
        _projectService.RegisterNodeType<AuthorsNode>();
        _projectService.RegisterNodeType<BitBucketNode>();
        _projectService.RegisterNodeType<BuildEventNode>();
        _projectService.RegisterNodeType<BuildOptionsNode>();
        _projectService.RegisterNodeType<BuildOutputNode>();
        _projectService.RegisterNodeType<BuildRuleNode>();
        _projectService.RegisterNodeType<CodegenNode>();
        _projectService.RegisterNodeType<CommandsNode>();
        _projectService.RegisterNodeType<ConfigurationNode>();
        _projectService.RegisterNodeType<DefinesNode>();
        _projectService.RegisterNodeType<DependenciesNode>();
        _projectService.RegisterNodeType<DialectNode>();
        _projectService.RegisterNodeType<ExceptionsNode>();
        _projectService.RegisterNodeType<ExternalNode>();
        _projectService.RegisterNodeType<FilesNode>();
        _projectService.RegisterNodeType<FloatingPointNode>();
        _projectService.RegisterNodeType<GitHubNode>();
        _projectService.RegisterNodeType<ImportNode>();
        _projectService.RegisterNodeType<IncludeDirsNode>();
        _projectService.RegisterNodeType<InputsNode>();
        _projectService.RegisterNodeType<InstallNode>();
        _projectService.RegisterNodeType<LinkOptionsNode>();
        _projectService.RegisterNodeType<ModuleNode>();
        _projectService.RegisterNodeType<NugetNode>();
        _projectService.RegisterNodeType<OptimizeNode>();
        _projectService.RegisterNodeType<OptionNode>();
        _projectService.RegisterNodeType<OutputsNode>();
        _projectService.RegisterNodeType<PlatformNode>();
        _projectService.RegisterNodeType<PluginNode>();
        _projectService.RegisterNodeType<PrivateNode>();
        _projectService.RegisterNodeType<ProjectNode>();
        _projectService.RegisterNodeType<PublicNode>();
        _projectService.RegisterNodeType<RuntimeNode>();
        _projectService.RegisterNodeType<SanitizeNode>();
        _projectService.RegisterNodeType<SymbolsNode>();
        _projectService.RegisterNodeType<SystemNode>();
        _projectService.RegisterNodeType<TagsNode>();
        _projectService.RegisterNodeType<ToolsetNode>();
        _projectService.RegisterNodeType<WarningsNode>();
        _projectService.RegisterNodeType<WhenNode>();

        return Task.CompletedTask;
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }
}
