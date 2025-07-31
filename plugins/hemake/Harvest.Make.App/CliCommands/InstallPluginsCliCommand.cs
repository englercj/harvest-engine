// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.CliCommands;
using Harvest.Make.Projects;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.Logging;
using System.CommandLine.Invocation;

namespace Harvest.Make.App.CliCommands;

[Service<ICliCommand>(Enumerable = true)]
internal class InstallPluginsCliCommand(
    ILogger<InstallPluginsCliCommand> logger,
    IProjectService projectService)
    : ICliCommand
{
    protected readonly ILogger _logger = logger;
    protected readonly IProjectService _projectService = projectService;

    public string Name => "install-plugins";
    public string Description => "Install plugins for the project.";

    public async Task<int> RunCommandAsync(InvocationContext context)
    {
        try
        {
            await InstallPluginsAsync(context);
            return 0;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "An error occurred while installing plugins.");
            return -1;
        }
    }

    private async Task InstallPluginsAsync(InvocationContext invocationContext)
    {
        ProjectContext baseContext = _projectService.CreateProjectContext(invocationContext);
        List<PluginNode> plugins = _projectService.GetNodes<PluginNode>(baseContext);
        List<ConfigurationNode> configurations = _projectService.GetNodes<ConfigurationNode>(baseContext);
        List<PlatformNode> platforms = _projectService.GetNodes<PlatformNode>(baseContext);

        List<Task> installTasks = [];

        foreach (ConfigurationNode configuration in configurations)
        {
            foreach (PlatformNode platform in platforms)
            {
                ProjectContext context = _projectService.CreateProjectContext(invocationContext);

                // TODO: install node may have unscoped install logic, which means we'd duplicate
                // the install logic for each configuration/platform combination.
                // Maybe we should try to get an install from baseContext first, then store each
                // install in a dictionary and only run the install once.
                foreach (PluginNode plugin in plugins)
                {
                    installTasks.Add(InstallPluginAsync(context, plugin));
                }
            }
        }

        await Task.WhenAll(installTasks);

        _logger.LogInformation("All plugins installed successfully.");
    }

    private async Task InstallPluginAsync(ProjectContext context, PluginNode plugin)
    {
        string dirName = Path.GetDirectoryName(plugin.Node.SourceInfo.FilePath) ?? string.Empty;
        // TODO: Logic for installing the plugin

        // Simulate plugin installation logic
        await Task.Delay(1000); // Simulate some async work
        _logger.LogInformation("Plugin {PluginName} installed successfully.", plugin.Name);
    }
}
