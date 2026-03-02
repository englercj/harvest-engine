// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Attributes;
using Harvest.Make.Projects;
using Harvest.Make.Projects.Nodes;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using System.CommandLine;
using System.Reflection;

namespace Harvest.Make.CLI;

class Program
{
    static async Task Main(string[] args)
    {
        // Initialize logging
        Log.Logger = new LoggerConfiguration()
            .WriteTo.Console(
                restrictedToMinimumLevel: GetLogLevel(appArgs),
                standardErrorFromLevel: LogEventLevel.Warning)
#if DEBUG
            .WriteTo.Debug(
                restrictedToMinimumLevel: LogEventLevel.Debug)
#endif
            .CreateLogger();

        // Configure services
        ServiceCollection services = new();

        services.AddLogging(builder => builder.AddSerilog(dispose: true));

        // Load the project early so we can use the configuration for initialization
        ILoggerFactory loggerFactory = new LoggerFactory().AddSerilog(Log.Logger);
        ProjectService projectService = new(appStorage, loggerFactory.CreateLogger<ProjectService>());

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

            string projectName = projectService.ProjectDocument.Nodes.FirstOrDefault()?.Arguments.FirstOrDefault()?.Value.GetValueString() ?? "<unknown>";
            Log.Information("Loaded project: {ProjectName}", projectName);
        }
        else
        {
            Log.Warning("No project loaded. Some commands may fail.");
        }

        // Load plugins
        PluginService pluginService = new(loggerFactory.CreateLogger<PluginService>(), projectService);
        pluginService.LoadPlugins();

        // Register services we created in Main
        services.AddSingleton<IProjectService>(_ => projectService);
        services.AddSingleton<IPluginService>(_ => pluginService);

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
}
