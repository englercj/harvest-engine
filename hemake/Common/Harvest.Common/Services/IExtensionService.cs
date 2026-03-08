using Harvest.Common;
using Microsoft.Extensions.DependencyInjection;
using System.Reflection;

namespace Harvest.Common.Services;

public interface IExtensionService
{
    public IReadOnlyList<IHarvestMakeExtension> Extensions { get; }

    void LoadExtensionsFromAppDomain(AppDomain appDomain);
    void LoadExtensionsFromAssembly(Assembly assembly);
    void LoadExtensionsFromAssemblyFile(string filePath);

    void ConfigureServices(IServiceCollection services);
    void Startup(IServiceProvider services);
    void Shutdown();
}
