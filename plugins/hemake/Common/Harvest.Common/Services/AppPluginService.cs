using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using System.Reflection;

namespace Harvest.Common.Services;

public class AppPluginService(ILogger<AppPluginService> logger) : IAppPluginService
{
    private IServiceProvider? _services;

    protected readonly List<IAppPlugin> _plugins = [];
    public IReadOnlyList<IAppPlugin> Plugins => _plugins.AsReadOnly();

    public void LoadPluginsFromAppDomain(AppDomain appDomain)
    {
        // Iterate all assemblies in the AppDomain and register any types that implement IAppPlugin.
        foreach (Assembly assembly in appDomain.GetAssemblies())
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

            LoadPluginsFromAssembly(assembly);
        }
    }

    public void LoadPluginsFromAssemblyFile(string filePath)
    {
        AppPluginLoadContext loadContext = new(filePath);
        Assembly assembly = loadContext.LoadFromAssemblyPath(filePath);
        LoadPluginsFromAssembly(assembly);
    }

    public void LoadPluginsFromAssembly(Assembly assembly)
    {
        foreach (Type type in assembly.GetTypes())
        {
            if (!type.IsClass || type.IsAbstract || !type.IsAssignableTo(typeof(IAppPlugin)))
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
}
