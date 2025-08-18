// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;

namespace Harvest.Make.Projects;

[Service<IAppLifetimeService>(Enumerable = true)]
public class NodeRegistrar(IProjectService projectService) : IAppLifetimeService
{
    private readonly IProjectService _projectService = projectService;

    public Task StartAsync(CancellationToken cancellationToken)
    {
        _projectService.RegisterNode<AuthorsNode>();
        _projectService.RegisterNode<BuildEventNode>();
        _projectService.RegisterNode<BuildOptionsNode>();
        _projectService.RegisterNode<BuildOutputNode>();
        _projectService.RegisterNode<BuildRuleNode>();
        _projectService.RegisterNode<CodegenNode>();
        _projectService.RegisterNode<CommandNode>();
        _projectService.RegisterNode<ConfigurationNode>();
        _projectService.RegisterNode<DefinesNode>();
        _projectService.RegisterNode<DependenciesNode>();
        _projectService.RegisterNode<DialectNode>();
        _projectService.RegisterNode<ExceptionsNode>();
        _projectService.RegisterNode<ExternalNode>();
        _projectService.RegisterNode<FetchNode>();
        _projectService.RegisterNode<FilesNode>();
        _projectService.RegisterNode<FloatingPointNode>();
        _projectService.RegisterNode<ImportNode>();
        _projectService.RegisterNode<IncludeDirsNode>();
        _projectService.RegisterNode<InputsNode>();
        _projectService.RegisterNode<InstallNode>();
        _projectService.RegisterNode<LibDirsNode>();
        _projectService.RegisterNode<LinkOptionsNode>();
        _projectService.RegisterNode<ModuleNode>();
        _projectService.RegisterNode<OptimizeNode>();
        _projectService.RegisterNode<OptionNode>();
        _projectService.RegisterNode<OutputsNode>();
        _projectService.RegisterNode<PlatformNode>();
        _projectService.RegisterNode<PluginNode>();
        _projectService.RegisterNode<ProjectNode>();
        _projectService.RegisterNode<PublicNode>();
        _projectService.RegisterNode<RuntimeNode>();
        _projectService.RegisterNode<SanitizeNode>();
        _projectService.RegisterNode<SymbolsNode>();
        _projectService.RegisterNode<SystemNode>();
        _projectService.RegisterNode<TagsNode>();
        _projectService.RegisterNode<ToolsetNode>();
        _projectService.RegisterNode<WarningsNode>();
        _projectService.RegisterNode<WhenNode>();

        _projectService.RegisterNodeGenerator<ForeachNodeGenerator>();

        _projectService.RegisterTokenResolver(ProjectNode.NodeTraits.Name, ProjectNode.TryResolveToken);
        _projectService.RegisterTokenResolver(PluginNode.NodeTraits.Name, PluginNode.TryResolveToken);
        _projectService.RegisterTokenResolver(ModuleNode.NodeTraits.Name, ModuleNode.TryResolveToken);

        return Task.CompletedTask;
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }
}
