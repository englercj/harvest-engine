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
        _projectService.RegisterNode<ArchiveNode>(ArchiveNode.NodeName);
        _projectService.RegisterNode<AuthorsNode>(AuthorsNode.NodeName);
        _projectService.RegisterNode<BitBucketNode>(BitBucketNode.NodeName);
        _projectService.RegisterNode<BuildEventNode>(BuildEventNode.NodeName);
        _projectService.RegisterNode<BuildOptionsNode>(BuildOptionsNode.NodeName);
        _projectService.RegisterNode<BuildOutputNode>(BuildOutputNode.NodeName);
        _projectService.RegisterNode<BuildRuleNode>(BuildRuleNode.NodeName);
        _projectService.RegisterNode<CodegenNode>(CodegenNode.NodeName);
        _projectService.RegisterNode<CommandsNode>(CommandsNode.NodeName);
        _projectService.RegisterNode<ConfigurationNode>(ConfigurationNode.NodeName);
        _projectService.RegisterNode<DefinesNode>(DefinesNode.NodeName);
        _projectService.RegisterNode<DependenciesNode>(DependenciesNode.NodeName);
        _projectService.RegisterNode<DialectNode>(DialectNode.NodeName);
        _projectService.RegisterNode<ExceptionsNode>(ExceptionsNode.NodeName);
        _projectService.RegisterNode<ExternalNode>(ExternalNode.NodeName);
        _projectService.RegisterNode<FilesNode>(FilesNode.NodeName);
        _projectService.RegisterNode<FloatingPointNode>(FloatingPointNode.NodeName);
        _projectService.RegisterNode<GitHubNode>(GitHubNode.NodeName);
        _projectService.RegisterNode<ImportNode>(ImportNode.NodeName);
        _projectService.RegisterNode<IncludeDirsNode>(IncludeDirsNode.NodeName);
        _projectService.RegisterNode<InputsNode>(InputsNode.NodeName);
        _projectService.RegisterNode<InstallNode>(InstallNode.NodeName);
        _projectService.RegisterNode<LinkOptionsNode>(LinkOptionsNode.NodeName);
        _projectService.RegisterNode<ModuleNode>(ModuleNode.NodeName);
        _projectService.RegisterNode<NugetNode>(NugetNode.NodeName);
        _projectService.RegisterNode<OptimizeNode>(OptimizeNode.NodeName);
        _projectService.RegisterNode<OptionNode>(OptionNode.NodeName);
        _projectService.RegisterNode<OutputsNode>(OutputsNode.NodeName);
        _projectService.RegisterNode<PlatformNode>(PlatformNode.NodeName);
        _projectService.RegisterNode<PluginNode>(PluginNode.NodeName);
        _projectService.RegisterNode<PrivateNode>(PrivateNode.NodeName);
        _projectService.RegisterNode<ProjectNode>(ProjectNode.NodeName);
        _projectService.RegisterNode<PublicNode>(PublicNode.NodeName);
        _projectService.RegisterNode<RuntimeNode>(RuntimeNode.NodeName);
        _projectService.RegisterNode<SanitizeNode>(SanitizeNode.NodeName);
        _projectService.RegisterNode<SymbolsNode>(SymbolsNode.NodeName);
        _projectService.RegisterNode<TagsNode>(TagsNode.NodeName);
        _projectService.RegisterNode<ToolsetNode>(ToolsetNode.NodeName);
        _projectService.RegisterNode<WarningsNode>(WarningsNode.NodeName);
        _projectService.RegisterNode<WhenNode>(WhenNode.NodeName);

        return Task.CompletedTask;
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }
}
