using Harvest.Kdl;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using System.Reflection;

namespace Harvest.Common.Services;

public class AppPluginService(ILogger<AppPluginService> logger, IProjectService projectService) : IAppPluginService
{
    private IServiceProvider? _services;

    protected readonly List<IAppPlugin> _plugins = [];
    public IReadOnlyList<IAppPlugin> Plugins => _plugins.AsReadOnly();

    public void LoadPlugins()
    {
        // Iterate all assemblies in the current AppDomain and register any types that implement IAppPlugin.
        foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
        {
            // Filter out large assemblies that are unlikely to contain plugins to speed up the process.
            if (assembly.GetName().Name is string assemblyName)
            {
                if (assemblyName.StartsWith("Microsoft", StringComparison.OrdinalIgnoreCase)
                    || assemblyName.StartsWith("System", StringComparison.OrdinalIgnoreCase)
                    || assemblyName.StartsWith("Windows", StringComparison.OrdinalIgnoreCase)
                    || assemblyName.StartsWith("netstandard", StringComparison.OrdinalIgnoreCase)
                    || assemblyName.StartsWith("mscorlib", StringComparison.OrdinalIgnoreCase)
                    || assemblyName.StartsWith("Serilog", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
            }

            CreatePluginsFromAssembly(assembly);
        }

        // Iterate the module nodes and load any hemake extensions
        foreach (KdlNode node in projectService.ProjectDocument.GetNodesByName(ModuleNode.NodeTraits.Name))
        {
            if (!node.TryGetValue("hemake_load", out bool isExt) || !isExt)
            {
                continue;
            }

            if (!node.TryGetValue("project_file", out string? extensionPath) || string.IsNullOrEmpty(extensionPath))
            {
                logger.LogError("Module '{ModuleName}' is marked as a hemake plugin but does not specify a 'project_file' property. This plugin cannot be loaded.", node.Name);
                continue;
            }

            if (!Path.IsPathRooted(extensionPath))
            {
                string extensionDir = Path.GetDirectoryName(node.SourceInfo.FilePath) ?? "";
                extensionPath = Path.Join(extensionDir, extensionPath);
            }

            Assembly assembly = LoadPluginAssembly(extensionPath);
            CreatePluginsFromAssembly(assembly);
        }
    }

    public void ConfigureServices(IServiceCollection services)
    {
        foreach (IAppPlugin plugin in Plugins)
        {
            plugin.ConfigureServices(services, logger);
        }
    }

    public void Startup(IServiceProvider services)
    {
        _services = services;

        foreach (IAppPlugin plugin in Plugins)
        {
            plugin.Startup(services);
        }
    }

    public void Shutdown()
    {
        foreach (IAppPlugin plugin in Plugins)
        {
            plugin.Shutdown();
        }

        _services = null;
    }

    private static Assembly LoadPluginAssembly(string extensionFullPath)
    {
        AppPluginLoadContext loadContext = new(extensionFullPath);
        return loadContext.LoadFromAssemblyPath(extensionFullPath);
    }

    private void CreatePluginsFromAssembly(Assembly assembly)
    {
        foreach (Type type in assembly.GetTypes())
        {
            if (!type.IsClass || type.IsAbstract || !type.IsAssignableTo<IAppPlugin>())
            {
                continue;
            }

            try
            {
                object? instance = Activator.CreateInstance(type);
                if (instance is not IAppPlugin plugin)
                {
                    logger.LogError("Failed to create plugin of type '{PluginType}' from assembly '{AssemblyName}'. Activator.CreateInstance returned null.", type.FullName, assembly.FullName);
                    continue;
                }

                _plugins.Add(plugin);
                logger.LogInformation("Loaded plugin '{PluginType}' from assembly '{AssemblyName}'.", type.FullName, assembly.FullName);
            }
            catch (Exception ex)
            {
                logger.LogError(ex, "Failed to create plugin of type '{PluginType}' from assembly '{AssemblyName}'.", type.FullName, assembly.FullName);
            }
        }
    }
}
