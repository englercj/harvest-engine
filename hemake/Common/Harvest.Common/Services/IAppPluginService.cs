using Microsoft.Extensions.DependencyInjection;
using System.Reflection;

namespace Harvest.Common.Services;

public interface IAppPluginService
{
    public IReadOnlyList<IAppPlugin> Plugins { get; }

    void LoadPluginsFromAppDomain(AppDomain appDomain);
    void LoadPluginsFromAssembly(Assembly assembly);
    void LoadPluginsFromAssemblyFile(string filePath);

    void ConfigureServices(IServiceCollection services);
    void Startup(IServiceProvider services);
    void Shutdown();
}
