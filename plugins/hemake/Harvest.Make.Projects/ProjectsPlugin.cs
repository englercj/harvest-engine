// Copyright Chad Engler

using Harvest.Common;
using Harvest.Make.Projects.NodeGenerators;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;

namespace Harvest.Make.Projects;

internal class ProjectsPlugin : IAppPlugin
{
    public void ConfigureServices(IServiceCollection services, ILogger logger)
    {
        services.AddAutoDiscoveredServices();
    }

    public void Startup(IServiceProvider services)
    {
        IProjectService projectService = services.GetRequiredService<IProjectService>();

        projectService.RegisterNode<AuthorsNode>();
        projectService.RegisterNode<BuildEventNode>();
        projectService.RegisterNode<BuildOptionsNode>();
        projectService.RegisterNode<BuildOutputNode>();
        projectService.RegisterNode<BuildRuleNode>();
        projectService.RegisterNode<CodegenNode>();
        projectService.RegisterNode<CommandNode>();
        projectService.RegisterNode<ConfigurationNode>();
        projectService.RegisterNode<DefinesNode>();
        projectService.RegisterNode<DependenciesNode>();
        projectService.RegisterNode<DialectNode>();
        projectService.RegisterNode<ExceptionsNode>();
        projectService.RegisterNode<ExternalNode>();
        projectService.RegisterNode<FetchNode>();
        projectService.RegisterNode<FilesNode>();
        projectService.RegisterNode<FloatingPointNode>();
        projectService.RegisterNode<ImportNode>();
        projectService.RegisterNode<IncludeDirsNode>();
        projectService.RegisterNode<InputsNode>();
        projectService.RegisterNode<InstallNode>();
        projectService.RegisterNode<LibDirsNode>();
        projectService.RegisterNode<LinkOptionsNode>();
        projectService.RegisterNode<ModuleNode>();
        projectService.RegisterNode<OptimizeNode>();
        projectService.RegisterNode<OptionNode>();
        projectService.RegisterNode<OutputsNode>();
        projectService.RegisterNode<PlatformNode>();
        projectService.RegisterNode<PluginNode>();
        projectService.RegisterNode<ProjectNode>();
        projectService.RegisterNode<PublicNode>();
        projectService.RegisterNode<RuntimeNode>();
        projectService.RegisterNode<SanitizeNode>();
        projectService.RegisterNode<SymbolsNode>();
        projectService.RegisterNode<SystemNode>();
        projectService.RegisterNode<TagsNode>();
        projectService.RegisterNode<ToolsetNode>();
        projectService.RegisterNode<WarningsNode>();
        projectService.RegisterNode<WhenNode>();

        projectService.RegisterNodeGenerator<ForeachNodeGenerator>();
    }

    public void Shutdown()
    {
        // TODO: Unregister nodes and generators?
    }
}
