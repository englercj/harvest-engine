// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Attributes;
using Harvest.Make.Projects;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using System.CommandLine;
using System.Reflection;

namespace Harvest.Make.App;

class Program
{
    static async Task Main(string[] args)
    {
        // Create and configure the application host builder
        HostApplicationBuilder builder = Host.CreateApplicationBuilder();
        builder.Environment.ApplicationName = "Harvest Engine Make";
#if DEBUG
        builder.Logging.AddDebug();
#endif

        ProjectService projectService = new();

        // Configure the services
        builder.Services.AllowResolvingKeyedServicesAsDictionary();
        builder.Services.AddHostedService<Application>();
        builder.Services.AddSingleton<IProjectService>(projectService);
        RegisterServicesFromAssembly(builder.Services, Assembly.GetExecutingAssembly());

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
        }

        // Iterate the module nodes and load any hemake extensions
        IEnumerable<KdlNode> moduleNodes = projectService.ProjectDocument.GetNodesByName("module");
        foreach (KdlNode node in moduleNodes)
        {
            if (!node.Properties.TryGetValue("hemake_extension", out KdlValue? hemakeExtensionValue)
                || hemakeExtensionValue is not KdlBool hemakeExtensionBool
                || !hemakeExtensionBool.Value)
            {
                continue;
            }

            if (!node.Properties.TryGetValue("project_file", out KdlValue? projectFileValue)
                || projectFileValue is not KdlString projectFileString)
            {
                throw new InvalidOperationException($"Module '{node.Name}' is marked as a hemake extension but does not specify a 'project_file' property.");
            }

            string extensionPath = projectFileString.Value;
            if (!Path.IsPathRooted(extensionPath))
            {
                string extensionDir = Path.GetDirectoryName(node.SourceInfo.FilePath) ?? string.Empty;
                extensionPath = Path.Combine(extensionDir, extensionPath);
            }

            Assembly assembly = LoadAppExtension(extensionPath);
            RegisterServicesFromAssembly(builder.Services, assembly);
        }

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
            if (attribute is not null)
            {
                RegisterService(services, type, attribute);
                continue;
            }
        }
    }

    static void RegisterService(IServiceCollection services, Type type, ServiceAttribute attribute)
    {
        if (attribute.Interfaces.Count == 0)
        {
            RegisterService(services, type, type, attribute);
        }
        else
        {
            foreach (Type interfaceType in attribute.Interfaces)
            {
                if (!type.IsAssignableTo(interfaceType))
                {
                    throw new InvalidOperationException($"Cannot register Service {type}. It specifies interface {interfaceType} in the Service attribute, but does not implement it.");
                }
                RegisterService(services, interfaceType, type, attribute);
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
