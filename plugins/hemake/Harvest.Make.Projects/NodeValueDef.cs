// Copyright Chad Engler

using Harvest.Kdl.Types;
using System.Numerics;

namespace Harvest.Make.Projects;

public class NodeValueDef(Type valueType)
{
    public Type ValueType { get; set; } = valueType;
    public bool IsRequired { get; set; } = false;
    public object? DefaultValue { get; set; } = null;
    public List<object> ValidValues { get; set; } = [];
}

public class NodeValueDef<T> : NodeValueDef
{
    public NodeValueDef() : base(typeof(T)) { }
}

public class NodeValueDef_Bool : NodeValueDef<KdlBool>
{
    public static NodeValueDef_Bool Required(bool defaultValue = false) => new() { DefaultValue = defaultValue, IsRequired = true };
    public static NodeValueDef_Bool Optional(bool? defaultValue = null) => new() { DefaultValue = defaultValue };
}

public class NodeValueDef_Enum<T> : NodeValueDef<KdlString> where T : struct, Enum
{
    protected static readonly List<object> s_validValues = KdlEnumUtils.GetNames<T>().ToList<object>();

    public static NodeValueDef_Enum<T> Required(T defaultValue) => new() { DefaultValue = defaultValue, ValidValues = s_validValues, IsRequired = true };
    public static NodeValueDef_Enum<T> Optional(T? defaultValue = null) => new() { DefaultValue = defaultValue, ValidValues = s_validValues };
}

public class NodeValueDef_Number<T> : NodeValueDef<KdlNumber<T>> where T : struct, INumber<T>
{
    public static NodeValueDef_Number<T> Required(T defaultValue = default) => new() { DefaultValue = defaultValue, IsRequired = true };
    public static NodeValueDef_Number<T> Optional(T? defaultValue = null) => new() { DefaultValue = defaultValue };
}

public class NodeValueDef_String : NodeValueDef<KdlString>
{
    public static NodeValueDef_String Required(string defaultValue = "") => new () { DefaultValue = defaultValue, IsRequired = true };
    public static NodeValueDef_String Optional(string? defaultValue = null) => new() { DefaultValue = defaultValue };
}

public class NodeValueDef_Path : NodeValueDef<KdlString>
{
    public static NodeValueDef_String Required(string defaultValue = "") => new() { DefaultValue = defaultValue, IsRequired = true };
    public static NodeValueDef_String Optional(string? defaultValue = null) => new() { DefaultValue = defaultValue };
}
