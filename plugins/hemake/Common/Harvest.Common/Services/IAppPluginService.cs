using Microsoft.Extensions.DependencyInjection;

namespace Harvest.Common.Services;

public interface IAppPluginService
{
    public IReadOnlyList<IAppPlugin> Plugins { get; }

    void LoadPlugins();

    void ConfigureServices(IServiceCollection services);
    void Startup(IServiceProvider services);
    void Shutdown();
}
