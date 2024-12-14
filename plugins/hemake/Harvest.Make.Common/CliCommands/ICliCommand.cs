// Copyright Chad Engler

using System.CommandLine.Invocation;

namespace Harvest.Make.CliCommands;

public interface ICliCommand
{
    public string Name { get; }
    public string Description { get; }

    public Task<int> RunCommandAsync(InvocationContext context);
}
