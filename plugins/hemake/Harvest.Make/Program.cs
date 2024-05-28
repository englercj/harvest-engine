// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.Services;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using System.Reflection;

namespace Harvest.Make;

class Program
{
    static async Task Main(string[] args)
    {
        // Create and configure the application host builder
        HostApplicationBuilder builder = Host.CreateApplicationBuilder();
        builder.Environment.ApplicationName = "Harvest Make";
#if DEBUG
        builder.Logging.AddDebug();
#endif

        // Manually find the `--project` option value.
        string? projectPath = null;
        int projectOptionIndex = Array.FindIndex(args, arg => arg == "--project");
        if (projectOptionIndex > -1 && projectOptionIndex < (args.Length - 1))
        {
            projectPath = args[projectOptionIndex + 1];
        }

        // Load up the project and plugin files
        ProjectService projectService = new();
        if (projectPath is not null)
        {
            await projectService.LoadProjectAsync(projectPath);
        }

        // Configure the services, including any services from plugins that provide extensions
        builder.Services.AllowResolvingKeyedServicesAsDictionary();
        builder.Services.AddHostedService<Application>();
        builder.Services.AddSingleton<IProjectService>(projectService);
        RegisterServicesFromAssembly(builder.Services, Assembly.GetExecutingAssembly());

        // TODO
        //foreach (IPlugin plugin in projectService.Plugins)
        //{
        //    foreach (IExtension extension in plugin.HemakeExtensions)
        //    {
        //        string extensionPath = Path.Combine(plugin.FilePath, extension);
        //        Assembly assembly = LoadAppExtension(extensionPath);
        //        RegisterServicesFromAssembly(builder.Services, assembly);
        //    }
        //}

        // Build the host and run the cli application
        using IHost host = builder.Build();
        await host.RunAsync();
    }

    static Assembly LoadAppExtension(string extensionFullPath)
    {
        AppExtensionContext loadContext = new(extensionFullPath);
        AssemblyName assemblyName = new(Path.GetFileNameWithoutExtension(extensionFullPath));
        return loadContext.LoadFromAssemblyName(assemblyName);
    }

    static void RegisterServicesFromAssembly(IServiceCollection services, Assembly assembly)
    {
        foreach (Type type in assembly.GetTypes())
        {
            if (!type.IsClass)
            {
                continue;
            }

            ServiceAttribute? attribute = type.GetCustomAttribute<ServiceAttribute>();
            if (attribute is null)
            {
                continue;
            }

            if (attribute.Interfaces.Count == 0)
            {
                RegisterService(services, type, type, attribute);
            }
            else
            {
                foreach (Type interfaceType in attribute.Interfaces)
                {
                    if (!interfaceType.IsAssignableFrom(type))
                    {
                        throw new InvalidOperationException($"Cannot register Service {type}. It specifies interface {interfaceType} in the Service attribute, but does not implement it.");
                    }
                    RegisterService(services, interfaceType, type, attribute);
                }
            }
        }
    }

    static void RegisterService(IServiceCollection services, Type interfaceType, Type serviceType, ServiceAttribute attribute)
    {
        ServiceDescriptor descriptor = new(interfaceType, attribute.Key, serviceType, attribute.Lifetime);
        if (attribute.Enumerable)
        {
            services.TryAddEnumerable(descriptor);
        }
        else
        {
            services.TryAdd(descriptor);
        }
    }
}
