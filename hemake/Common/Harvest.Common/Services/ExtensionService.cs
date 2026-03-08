using Harvest.Common;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using System.Reflection;

namespace Harvest.Common.Services;

public class ExtensionService(ILogger<ExtensionService> logger) : IExtensionService
{
    private IServiceProvider? _services;

    private readonly List<IHarvestMakeExtension> _extensions = [];
    public IReadOnlyList<IHarvestMakeExtension> Extensions => _extensions.AsReadOnly();

    public void LoadExtensionsFromAppDomain(AppDomain appDomain)
    {
        // Iterate all assemblies in the AppDomain and register any types that implement IHarvestMakeExtension.
        foreach (Assembly assembly in appDomain.GetAssemblies())
        {
            // Filter out large assemblies that are unlikely to contain HE Make extensions to speed up the scan.
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

            LoadExtensionsFromAssembly(assembly);
        }
    }

    public void LoadExtensionsFromAssemblyFile(string filePath)
    {
        ExtensionLoadContext loadContext = new(filePath);
        Assembly assembly = loadContext.LoadFromAssemblyPath(filePath);
        LoadExtensionsFromAssembly(assembly);
    }

    public void LoadExtensionsFromAssembly(Assembly assembly)
    {
        foreach (Type type in assembly.GetTypes())
        {
            if (!type.IsClass || type.IsAbstract || !type.IsAssignableTo(typeof(IHarvestMakeExtension)))
            {
                continue;
            }

            try
            {
                object? instance = Activator.CreateInstance(type);
                if (instance is not IHarvestMakeExtension extension)
                {
                    logger.LogError("Failed to create HE Make extension of type '{ExtensionType}' from assembly '{AssemblyName}'. Activator.CreateInstance returned null.", type.FullName, assembly.FullName);
                    continue;
                }

                _extensions.Add(extension);
                logger.LogInformation("Loaded HE Make extension '{ExtensionType}' from assembly '{AssemblyName}'.", type.Name, assembly.GetName().Name);
            }
            catch (Exception ex)
            {
                logger.LogError(ex, "Failed to create HE Make extension of type '{ExtensionType}' from assembly '{AssemblyName}'.", type.FullName, assembly.FullName);
            }
        }
    }

    public void ConfigureServices(IServiceCollection services)
    {
        foreach (IHarvestMakeExtension extension in Extensions)
        {
            extension.ConfigureServices(services, logger);
        }
    }

    public void Startup(IServiceProvider services)
    {
        _services = services;

        foreach (IHarvestMakeExtension extension in Extensions)
        {
            extension.Startup(services);
        }
    }

    public void Shutdown()
    {
        foreach (IHarvestMakeExtension extension in Extensions)
        {
            extension.Shutdown();
        }

        _services = null;
    }
}
