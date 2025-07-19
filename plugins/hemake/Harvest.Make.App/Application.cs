// Copyright Chad Engler

using Harvest.Make.Attributes;
using Harvest.Make.CliCommands;
using Harvest.Make.Commands;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using System.CommandLine;
using System.Reflection;
using System.Collections;
using System.CommandLine.Parsing;
using System.CommandLine.Invocation;
using Harvest.Make.Projects;

namespace Harvest.Make.App;

[Service<IHostedService>(Enumerable = true)]
internal class Application : BackgroundService
{
    private readonly ILogger _logger;
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
        IEnumerable<ICliCommand> commands,
        IHostApplicationLifetime appLifetime,
        ILogger<Application> logger,
        IProjectService projectService,
        IServiceProvider serviceProvider)
    {
        _logger = logger;
        _appLifetime = appLifetime;
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

        // Options specified in the project file are added as global options available to all commands.
        foreach (ProjectOption projectOption in projectService.Options)
        {
            _rootCommand.AddGlobalOption(projectOption.Option);
        }

        SetupCommands(commands);
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

    private void SetupCommands(IEnumerable<ICliCommand> commands)
    {
        foreach (ICliCommand command in commands)
        {
            Command cliCommand = new(command.Name, command.Description);

            CommandInfo commandInfo = new(command.GetType());

            foreach (PropertyInfo property in commandInfo.CommandType.GetProperties())
            {
                if (property.GetCustomAttribute<CliOptionAttribute>() is CliOptionAttribute optionInfo)
                {
                    Type optionType = s_genericOptionType.MakeGenericType(property.PropertyType);
                    if (Activator.CreateInstance(optionType, optionInfo.Name, optionInfo.Description) is Option option)
                    {
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
                }

                if (property.GetCustomAttribute<CliArgumentAttribute>() is CliArgumentAttribute argumentInfo)
                {
                    Type argumentType = s_genericArgumentType.MakeGenericType(property.PropertyType);
                    if (Activator.CreateInstance(argumentType, argumentInfo.Name, argumentInfo.Description) is Argument argument)
                    {
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

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        try
        {
            string[] args = Environment.GetCommandLineArgs();
            Environment.ExitCode = await _rootCommand.InvokeAsync(args);
        }
        catch (Exception ex)
        {
            Environment.ExitCode = -1;
            _logger.LogError(ex, "An error occurred while executing the command.");
        }

        _appLifetime.StopApplication();
    }
}
