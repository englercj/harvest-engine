// Copyright Chad Engler

using Harvest.Common;
using Harvest.Common.Services;
using Harvest.Kdl;
using Harvest.Make.Projects.Nodes;
using Harvest.Make.Projects.Services;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Serilog;
using Serilog.Core;
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

        // Load HE Make extensions from the current AppDomain
        ExtensionService extensionService = new(loggerFactory.CreateLogger<ExtensionService>());
        extensionService.LoadExtensionsFromAppDomain(AppDomain.CurrentDomain);

        // Discover HE Make extension projects from the project file and build them.
        // We have to do this before parsing the project because the extensions may define custom
        // nodes that are used in the project structure.
        DotnetBuildService dotnetBuildService = new(loggerFactory.CreateLogger<DotnetBuildService>());
        HashSet<string> seenProjectFiles = [];
        List<(string, Task<DotnetBuildResult>)> pendingProjectBuilds = [];

        foreach (KdlNode node in projectService.ProjectDocument.GetNodesByName(ModuleNode.NodeTraits.Name))
        {
            ModuleNode module = new(node);
            if (module.Kind != EModuleKind.HarvestMakeExtension)
            {
                continue;
            }

            string moduleName = node.TryGetValue(0, out string? explicitModuleName) && !string.IsNullOrEmpty(explicitModuleName)
                ? explicitModuleName
                : node.Name;

            if (!node.TryGetValue("project_file", out string? projectFilePath) || string.IsNullOrWhiteSpace(projectFilePath))
            {
                Log.Error("Module '{ModuleName}' is an HE Make extension but does not specify a 'project_file' property. This extension cannot be loaded.", moduleName);
                continue;
            }

            projectFilePath = Path.IsPathRooted(projectFilePath)
                ? Path.GetFullPath(projectFilePath)
                : Path.GetFullPath(projectFilePath, Path.GetDirectoryName(node.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory());

            if (!seenProjectFiles.Add(projectFilePath))
            {
                continue;
            }

            pendingProjectBuilds.Add((moduleName, dotnetBuildService.BuildProjectAsync(projectFilePath)));
        }

        if (pendingProjectBuilds.Count > 0)
        {
            foreach ((string moduleName, Task<DotnetBuildResult> buildTask) in pendingProjectBuilds)
            {
                try
                {
                    DotnetBuildResult result = await buildTask;
                    extensionService.LoadExtensionsFromAssemblyFile(result.AssemblyPath);
                }
                catch (Exception ex)
                {
                    Log.Error(ex, "Module '{ModuleName}' failed to load HE Make extension.", moduleName);
                }
            }
        }

        // Register services we created in Main
        services.AddSingleton<IExtensionService>(_ => extensionService);
        services.AddSingleton<IDotnetBuildService>(_ => dotnetBuildService);
        services.AddSingleton<IProjectService>(_ => projectService);

        // Register services from our assembly that are auto-discovered based on attributes
        services.AddAutoDiscoveredServices();

        // Allow loaded HE Make extensions to configure additional services
        extensionService.ConfigureServices(services);

        // Build the service provider
        ServiceProvider serviceProvider = services.BuildServiceProvider();

        // Startup HE Make extensions now that the service provider has been built
        extensionService.Startup(serviceProvider);

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
                extensionService.Shutdown();
            }
            catch (Exception ex)
            {
                Log.Fatal(ex, "An unhandled exception occurred while shutting down HE Make extensions.");
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
