// Copyright Chad Engler

using DotMake.CommandLine;
using Harvest.Make.Attributes;
using Harvest.Make.Commands;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;

namespace Harvest.Make;

[Service<IHostedService>(Enumerable = true)]
internal class Application : BackgroundService
{
    private readonly ILogger _logger;
    private readonly IHostApplicationLifetime _appLifetime;
    private readonly IServiceProvider _serviceProvider;

    public Application(
        ILogger<Application> logger,
        IHostApplicationLifetime appLifetime,
        IServiceProvider serviceProvider)
    {
        _logger = logger;
        _appLifetime = appLifetime;
        _serviceProvider = serviceProvider;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        string[] args = Cli.GetArgs();

        Cli.Ext.SetServiceProvider(_serviceProvider);

        try
        {
            Environment.ExitCode = await Cli.RunAsync<RootCliCommand>(args, null, stoppingToken);
        }
        catch (Exception ex)
        {
            Environment.ExitCode = 1;
            _logger.LogError(ex, "An error occurred while executing the command.");
        }

        _appLifetime.StopApplication();
    }
}
