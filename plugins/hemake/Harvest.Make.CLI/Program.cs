// Copyright Chad Engler

using Harvest.Common;
using Harvest.Common.Services;
using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Serilog;
using Serilog.Events;
using System.CommandLine;

namespace Harvest.Make.CLI;

class Program
{
    static async Task<int> Main(string[] args)
    {
        // Initialize logging
        Log.Logger = new LoggerConfiguration()
            .WriteTo.Console(
                restrictedToMinimumLevel: GetLogLevel(),
                standardErrorFromLevel: LogEventLevel.Warning)
#if DEBUG
            .WriteTo.Debug(
                restrictedToMinimumLevel: LogEventLevel.Debug)
#endif
            .CreateLogger();

        Log.Information("Running with: {Args}", string.Join(' ', args));

        // Configure services
        ServiceCollection services = new();

        services.AddLogging(builder => builder.AddSerilog(dispose: true));

        // Load the project early so we can use the configuration for initialization
        ILoggerFactory loggerFactory = new LoggerFactory().AddSerilog(Log.Logger);
        ProjectService projectService = new();

        // Manually find the `--project` option value.
        string? projectPath = null;
        int projectOptionIndex = Array.FindIndex(args, arg => arg == "--project");
        if (projectOptionIndex > -1 && projectOptionIndex < (args.Length - 1))
        {
            projectPath = args[projectOptionIndex + 1];
        }

        // Load up the project files. This will load the KDL project structure but not
        // parse it into semantic nodes yet. We need to load any hemake extensions first.
        if (projectPath is not null)
        {
            projectService.LoadProject(projectPath);

            string projectName = projectService.ProjectDocument.Nodes.FirstOrDefault()?.Arguments.FirstOrDefault()?.GetValueString() ?? "<unknown>";
            Log.Information("Loaded project: {ProjectName}", projectName);
        }
        else
        {
            Log.Warning("No project loaded. Some commands may fail.");
        }

        // Load plugins
        AppPluginService pluginService = new(loggerFactory.CreateLogger<AppPluginService>());
        pluginService.LoadPluginsFromAppDomain(AppDomain.CurrentDomain);

        // Iterate the module nodes and load any hemake extensions
        foreach (KdlNode node in projectService.ProjectDocument.GetNodesByName(ModuleNode.NodeTraits.Name))
        {
            if (!node.TryGetValue("hemake_load", out bool isExt) || !isExt)
            {
                continue;
            }

            if (!node.TryGetValue("project_file", out string? extensionPath) || string.IsNullOrEmpty(extensionPath))
            {
                Log.Error("Module '{ModuleName}' is marked as a hemake plugin but does not specify a 'project_file' property. This plugin cannot be loaded.", node.Name);
                continue;
            }

            if (!Path.IsPathRooted(extensionPath))
            {
                string extensionDir = Path.GetDirectoryName(node.SourceInfo.FilePath) ?? "";
                extensionPath = Path.Join(extensionDir, extensionPath);
            }

            pluginService.LoadPluginsFromAssemblyFile(extensionPath);
        }

        // Register services we created in Main
        services.AddSingleton<IAppPluginService>(_ => pluginService);
        services.AddSingleton<IProjectService>(_ => projectService);

        // Register services from our assembly that are auto-discovered based on attributes
        services.AddAutoDiscoveredServices();

        // Allow loaded plugins to configure additional services
        pluginService.ConfigureServices(services);

        // Build the service provider
        ServiceProvider serviceProvider = services.BuildServiceProvider();

        // Startup plugins now that the service provider has been built
        pluginService.Startup(serviceProvider);

        // Build the CLI command structure
        RootCommand rootCommand = new("HEMake CLI");

        // Add global options (so they appear in help)
        rootCommand.Options.Add(new Option<string>("--project") { Description = "Path to the project file (*.he_project)." });

        // Discover commands
        IEnumerable<ICommandProvider> commandProviders = serviceProvider.GetServices<ICommandProvider>();
        foreach (ICommandProvider provider in commandProviders)
        {
            rootCommand.Add(provider.GetCommand());
        }

        // Parse and execute the root command
        try
        {
            ParseResult parseResult = rootCommand.Parse(args);

            if (projectPath is not null)
            {
                projectService.ParseProject(parseResult);
            }

            return await parseResult.InvokeAsync(new InvocationConfiguration()
            {
                EnableDefaultExceptionHandler = false,
            });
        }
        catch (Exception ex)
        {
            Log.Fatal(ex, "An unhandled exception occurred while executing the command.");
            return 1;
        }
        finally
        {
            try
            {
                pluginService.Shutdown();
            }
            catch (Exception ex)
            {
                Log.Fatal(ex, "An unhandled exception occurred while shutting down plugins.");
            }
        }
    }

    private static LogEventLevel GetLogLevel()
    {
#if DEBUG
        return LogEventLevel.Debug;
#else
        return LogEventLevel.Information;
#endif
    }
}
