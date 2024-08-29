// Copyright Chad Engler

namespace Harvest.Make.CliCommands;

public interface ICliCommand
{
    public Task<int> RunCommandAsync();
    public void ValidateCommand();
}
