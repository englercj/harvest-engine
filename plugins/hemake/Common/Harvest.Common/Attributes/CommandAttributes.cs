namespace Harvest.Common.Attributes;

// Attributes in this file are used to mark services and view models for
// the source generator to generate command provider code.

/// <summary>
/// An Attribute that indicates that a given class implements a CLI command. The class must be
/// partial and implement the <see cref="ICommandExecutor"/> interface. The class will be
/// augmented by the source generator to implement the <see cref="ICommandProvider"/> interface,
/// as well as added to the list of automatically discovered services.
/// <para>
/// This attribute can be used as follows:
/// <code>
/// [Command("Description of the command")]
/// partial class MyCommand : ICommandExecutor
/// {
///     public Task<int> ExecuteAsync(CancellationToken ct) { return 0; }
/// }
/// </code>
/// </para>
/// </summary>
/// <remarks>
/// The default name of the command is the class name converted to kebab-case with any
/// "Command" suffix removed.
/// </remarks>
[AttributeUsage(AttributeTargets.Class, Inherited = false, AllowMultiple = false)]
public sealed class CommandAttribute(string? description = null, string? name = null) : Attribute
{
    public string? Name { get; } = name;
    public string? Description { get; } = description;
}

/// <summary>
/// Marks a property as a CLI Option. The property will be filled with the value of the
/// corresponding option when the command is executed.
/// <para>
/// This attribute can be used as follows:
/// <code>
/// [Command("Description of the command")]
/// partial class MyCommand : ICommandExecutor
/// {
///     [Option("Description of the option")]
///     public string SomeValue { get; set; } = "default_value";
///
///     public Task<int> ExecuteAsync(CancellationToken ct) { return 0; }
/// }
/// </code>
/// </para>
/// </summary>
/// <remarks>
/// The default name of the option is the property name converted to kebab-case with any
/// "Option" suffix removed.
/// </remarks>
[AttributeUsage(AttributeTargets.Property, Inherited = false, AllowMultiple = false)]
public sealed class OptionAttribute(string? description = null, string? name = null) : Attribute
{
    public string? Name { get; } = name;
    public string? Description { get; } = description;
    public string[]? Aliases { get; set; }
    public bool IsRequired { get; set; } = false;
}

/// <summary>
/// Marks a property as a CLI Argument. The property will be filled with the value of the
/// corresponding argument when the command is executed.
/// <para>
/// This attribute can be used as follows:
/// <code>
/// [Command("Description of the command")]
/// partial class MyCommand : ICommandExecutor
/// {
///     [Argument("Description of the argument")]
///     public string SomeValue { get; set; } = "default_value";
///
///     public Task<int> ExecuteAsync(CancellationToken ct) { return 0; }
/// }
/// </code>
/// </para>
/// </summary>
/// <remarks>
/// The default name of the argument is the property name converted to kebab-case with any
/// "Argument" or "Arg" suffix removed.
/// </remarks>
[AttributeUsage(AttributeTargets.Property, Inherited = false, AllowMultiple = false)]
public sealed class ArgumentAttribute(string? description = null, string? name = null) : Attribute
{
    public string? Name { get; } = name;
    public string? Description { get; } = description;
    public ArgArity Arity { get; set; }
}

public enum ArgArity
{
    /// <summary>
    /// An arity that does not allow any values.
    /// </summary>
    Zero,

    /// <summary>
    /// An arity that may have one value, but no more than one.
    /// </summary>
    ZeroOrOne,

    /// <summary>
    /// An arity that must have exactly one value.
    /// </summary>
    ExactlyOne,

    /// <summary>
    /// An arity that may have multiple values.
    /// </summary>
    ZeroOrMore,

    /// <summary>
    /// An arity that must have at least one value.
    /// </summary>
    OneOrMore,
}
