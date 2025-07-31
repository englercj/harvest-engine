// Copyright Chad Engler

namespace Harvest.Make;

public interface IAppLifetimeService
{
    public Task StartAsync(CancellationToken cancellationToken);
    public Task StopAsync(CancellationToken cancellationToken);
}
