// Copyright Chad Engler

using Harvest.Make.App.CliCommands;
using Harvest.Make.Attributes;
using Harvest.Make.CliCommands;
using Harvest.Make.Projects;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using System.Collections;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.CommandLine.Parsing;
using System.Reflection;

namespace Harvest.Make.App;

[Service<IHostedService>(Enumerable = true)]
internal class Application : IHostedService
{
    private readonly ILogger _logger;
    private readonly IProjectService _projectService;
    private readonly IEnumerable<IAppLifetimeService> _appLifetimeServices;
    private readonly IEnumerable<ICliCommand> _commands;
    private readonly IHostApplicationLifetime _appLifetime;
    private readonly IServiceProvider _serviceProvider;
    private readonly RootCommand _rootCommand;
    private readonly Option<FileInfo> _projectOption;

    private static readonly Type s_genericOptionType = typeof(Option<>);
    private static readonly Type s_genericArgumentType = typeof(Argument<>);

    private delegate void SetCommandValueDelegate(object command, object value);

    private class CommandBinding(Symbol symbol, SetCommandValueDelegate valueSetter)
    {
        public Symbol Symbol { get; set; } = symbol;
        public SetCommandValueDelegate ValueSetter { get; set; } = valueSetter;
    }

    private class CommandInfo(Type commandType)
    {
        public Type CommandType { get; set; } = commandType;
        public List<CommandBinding> Bindings { get; set; } = [];
    }

    public Application(
        IEnumerable<IAppLifetimeService> appLifetimeServices,
        IEnumerable<ICliCommand> commands,
        IHostApplicationLifetime appLifetime,
        ILogger<Application> logger,
        IProjectService projectService,
        IServiceProvider serviceProvider)
    {
        _appLifetimeServices = appLifetimeServices;
        _commands = commands;
        _appLifetime = appLifetime;
        _logger = logger;
        _projectService = projectService;
        _serviceProvider = serviceProvider;

        _rootCommand = new RootCliCommand()
        {
            Name = "hemake",
            Description = "Harvest Engine Make"
        };

        // The project option is a bit special. We want to use it in Program.cs to load the
        // project file before we start the application. This is because the project file may
        // include extensions to hemake itself. This option is handled manually in Program.cs,
        // but we still want to define it here so there aren't errors about `--project` not
        // being a valid option.
        //
        // Commands that need the project file path can use the IProjectService.
        _projectOption = new Option<FileInfo>("--project", "The Harvest Engine project file for your project.");
        _rootCommand.AddGlobalOption(_projectOption);
    }

    public async Task StartAsync(CancellationToken cancellationToken)
    {
        // Start up the application services
        foreach (IAppLifetimeService appLifetimeService in _appLifetimeServices)
        {
            await appLifetimeService.StartAsync(cancellationToken);

            if (cancellationToken.IsCancellationRequested)
            {
                Environment.ExitCode = -1;
                _appLifetime.StopApplication();
                return;
            }
        }

        // Parse the project file into semantic nodes now that all extensions are loaded
        try
        {
            _projectService.ParseProject();
            SetupCommands();
        }
        catch (Exception ex)
        {
            Environment.ExitCode = -1;
            _logger.LogError(ex, "An error occurred while parsing the project structure.");
            _appLifetime.StopApplication();
            return;
        }

        // Run the command
        try
        {
            string[] args = Environment.GetCommandLineArgs();
            Environment.ExitCode = await _rootCommand.InvokeAsync(args[1..]);
        }
        catch (Exception ex)
        {
            Environment.ExitCode = -1;
            _logger.LogError(ex, "An error occurred while executing the command.");
        }

        _appLifetime.StopApplication();
    }

    public async Task StopAsync(CancellationToken cancellationToken)
    {
        foreach (IAppLifetimeService appLifetimeService in _appLifetimeServices)
        {
            await appLifetimeService.StopAsync(cancellationToken);

            if (cancellationToken.IsCancellationRequested)
            {
                break;
            }
        }
    }

    private static ArgumentArity GetArityForProperty(PropertyInfo property)
    {
        if (property.PropertyType == typeof(bool))
        {
            return ArgumentArity.ZeroOrOne;
        }

        if (property.PropertyType.IsAssignableTo(typeof(IEnumerable)))
        {
            return ArgumentArity.ZeroOrMore;
        }

        return ArgumentArity.ExactlyOne;
    }

    private void SetupCommands()
    {
        foreach (ProjectOption projectOption in _projectService.ProjectOptions)
        {
            _rootCommand.AddGlobalOption(projectOption.Option);
        }

        foreach (ICliCommand command in _commands)
        {
            Command cliCommand = new(command.Name, command.Description);

            CommandInfo commandInfo = new(command.GetType());

            foreach (PropertyInfo property in commandInfo.CommandType.GetProperties())
            {
                if (property.GetCustomAttribute<CliOptionAttribute>() is CliOptionAttribute optionInfo)
                {
                    Type optionType = s_genericOptionType.MakeGenericType(property.PropertyType);
                    Option option = Activator.CreateInstance(optionType, optionInfo.Name, optionInfo.Description) as Option
                        ?? throw new Exception($"Failed to create an instance of option type {optionType.FullName} for property {property.Name} on command {commandInfo.CommandType.FullName}");

                    foreach (string alias in optionInfo.Aliases)
                    {
                        option.AddAlias(alias);
                    }

                    option.IsHidden = optionInfo.IsHidden;
                    option.IsRequired = optionInfo.IsRequired;
                    option.Arity = GetArityForProperty(property);

                    if (!optionInfo.IsRequired)
                    {
                        option.SetDefaultValue(property.GetValue(command));
                    }

                    commandInfo.Bindings.Add(new CommandBinding(option, property.SetValue));
                    cliCommand.AddOption(option);
                }

                if (property.GetCustomAttribute<CliArgumentAttribute>() is CliArgumentAttribute argumentInfo)
                {
                    Type argumentType = s_genericArgumentType.MakeGenericType(property.PropertyType);
                    Argument argument = Activator.CreateInstance(argumentType, argumentInfo.Name, argumentInfo.Description) as Argument
                        ?? throw new Exception($"Failed to create an instance of argument type {argumentType.FullName} for property {property.Name} on command {commandInfo.CommandType.FullName}");

                    argument.IsHidden = argumentInfo.IsHidden;
                    argument.Arity = GetArityForProperty(property);

                    if (argumentInfo.IsRequired)
                    {
                        argument.AddValidator((result) =>
                        {
                            if (result.Tokens.Count == 0)
                            {
                                result.ErrorMessage = $"Missing required argument: {argumentInfo.Name}";
                            }
                        });
                    }
                    else
                    {
                        argument.SetDefaultValue(property.GetValue(command));
                    }

                    commandInfo.Bindings.Add(new CommandBinding(argument, property.SetValue));
                    cliCommand.AddArgument(argument);
                }
            }

            cliCommand.SetHandler((context) => HandleCommandAsync(context, commandInfo));

            _rootCommand.AddCommand(cliCommand);
        }
    }

    private async Task<int> HandleCommandAsync(InvocationContext context, CommandInfo commandInfo)
    {
        if (ActivatorUtilities.CreateInstance(_serviceProvider, commandInfo.CommandType) is ICliCommand command)
        {
            foreach (CommandBinding binding in commandInfo.Bindings)
            {
                object? value = binding.Symbol switch
                {
                    Argument argument => context.ParseResult.GetValueForArgument(argument),
                    Option option => context.ParseResult.GetValueForOption(option),
                    _ => throw new Exception("Unknown value binding symbol type: not an option or argument"),
                };

                if (value is not null)
                {
                    binding.ValueSetter(command, value);
                }
            }

            context.ExitCode = await command.RunCommandAsync(context);
        }
        else
        {
            _logger.LogError("Failed to create an instance of the command, or the command does not implement {InterfaceName}: {CommandName}", nameof(ICliCommand), commandInfo.CommandType.Name);
            context.ExitCode = -1;
        }

        return context.ExitCode;
    }
}
