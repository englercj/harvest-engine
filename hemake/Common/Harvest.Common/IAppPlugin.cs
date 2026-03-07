using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;

namespace Harvest.Common;

public interface IAppPlugin
{
    void ConfigureServices(IServiceCollection services, ILogger logger);
    void Startup(IServiceProvider services);
    void Shutdown();
}
