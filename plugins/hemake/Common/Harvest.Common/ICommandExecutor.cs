namespace Harvest.Common;

public interface ICommandExecutor
{
    Task<int> ExecuteAsync(CancellationToken ct);
}
