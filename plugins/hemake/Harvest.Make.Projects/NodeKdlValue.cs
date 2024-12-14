// Copyright Chad Engler

using Harvest.Kdl.Types;
using System.Numerics;

namespace Harvest.Make.Projects;

public class NodeKdlValue(Type valueType)
{
    public Type ValueType { get; set; } = valueType;
    public bool IsRequired { get; set; } = false;
    public object? DefaultValue { get; set; } = null;
    public List<object> ValidValues { get; set; } = [];
}

public class NodeKdlValue<T> : NodeKdlValue
{
    public NodeKdlValue() : base(typeof(T)) { }
}

public class NodeKdlBool : NodeKdlValue<KdlBool>
{
    public static NodeKdlBool Required(bool defaultValue = false) => new() { DefaultValue = defaultValue, IsRequired = true };
    public static NodeKdlBool Optional(bool? defaultValue = null) => new() { DefaultValue = defaultValue };
}

public class NodeKdlEnum<T> : NodeKdlValue<KdlString> where T : struct, Enum
{
    protected static readonly List<object> s_validValues = KdlEnumUtils.GetNames<T>().ToList<object>();

    public static NodeKdlEnum<T> Required(T defaultValue) => new() { DefaultValue = defaultValue, ValidValues = s_validValues, IsRequired = true };
    public static NodeKdlEnum<T> Optional(T? defaultValue = null) => new() { DefaultValue = defaultValue, ValidValues = s_validValues };
}

public class NodeKdlNumber<T> : NodeKdlValue<KdlNumber<T>> where T : struct, INumber<T>
{
    public static NodeKdlNumber<T> Required(T defaultValue = default) => new() { DefaultValue = defaultValue, IsRequired = true };
    public static NodeKdlNumber<T> Optional(T? defaultValue = null) => new() { DefaultValue = defaultValue };
}

public class NodeKdlString : NodeKdlValue<KdlString>
{
    public static NodeKdlString Required(string defaultValue = "") => new () { DefaultValue = defaultValue, IsRequired = true };
    public static NodeKdlString Optional(string? defaultValue = null) => new() { DefaultValue = defaultValue };
}

public class NodeKdlPath : NodeKdlValue<KdlString>
{
    public static NodeKdlString Required(string defaultValue = "") => new() { DefaultValue = defaultValue, IsRequired = true };
    public static NodeKdlString Optional(string? defaultValue = null) => new() { DefaultValue = defaultValue };
}
