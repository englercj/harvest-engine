// Copyright Chad Engler

using System.Reflection;
using System.Runtime.Loader;

namespace Harvest.Common;

internal sealed class AppPluginLoadContext(string path)
    : AssemblyLoadContext(name: "Harvest App Plugin Load Context", isCollectible: false)
{
    private readonly AssemblyDependencyResolver _resolver = new(path);
    private readonly AssemblyLoadContext _defaultLoadContext = GetLoadContext(Assembly.GetExecutingAssembly()) ?? Default;

    protected override Assembly? Load(AssemblyName assemblyName)
    {
        // First, try to load the assembly from the default load context.
        try
        {
            Assembly? assembly = _defaultLoadContext.LoadFromAssemblyName(assemblyName);
            if (assembly is not null)
            {
                return assembly;
            }
        }
        catch (FileNotFoundException)
        {
            // Ignore if the assembly is not found in the default context.
        }

        // Next, try to resolve the assembly using the resolver.
        string? assemblyPath = _resolver.ResolveAssemblyToPath(assemblyName);
        if (!string.IsNullOrEmpty(assemblyPath))
        {
            return LoadFromAssemblyPath(assemblyPath);
        }

        // If the assembly cannot be found, return null.
        return null;
    }

    protected override nint LoadUnmanagedDll(string unmanagedDllName)
    {
        string? libraryPath = _resolver.ResolveUnmanagedDllToPath(unmanagedDllName);
        if (!string.IsNullOrEmpty(libraryPath))
        {
            return LoadUnmanagedDllFromPath(libraryPath);
        }
        return nint.Zero;
    }
}
