// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using System.Numerics;

namespace Harvest.Make.Projects;

public abstract class NodeValueDef(Type valueType)
{
    public Type ValueType { get; set; } = valueType;
    public bool IsRequired { get; set; } = false;
    public KdlValue DefaultValue { get; set; } = new KdlNull();
    public List<object> ValidValues { get; set; } = [];
}

public abstract class NodeValueDef<T> : NodeValueDef where T : KdlValue
{
    public NodeValueDef() : base(typeof(T)) { }

    public T? TypedDefaultValue
    {
        get => DefaultValue as T;
        set => DefaultValue = value is not null ? value : new KdlNull();
    }
}

public class NodeValueDef_Bool : NodeValueDef<KdlBool>
{
    public static NodeValueDef_Bool Required(bool defaultValue = false) => new()
    {
        TypedDefaultValue = new KdlBool(defaultValue),
        IsRequired = true,
    };

    public static NodeValueDef_Bool Optional(bool? defaultValue = null) => new()
    {
        TypedDefaultValue = defaultValue.HasValue ? new KdlBool(defaultValue.Value) : null,
    };
}

public class NodeValueDef_Enum<T> : NodeValueDef<KdlString> where T : struct, Enum
{
    protected static readonly List<object> s_validValues = [.. KdlEnumUtils.GetNames<T>()];

    public static NodeValueDef_Enum<T> Required(T defaultValue) => new()
    {
        TypedDefaultValue = new KdlString(KdlEnumUtils.GetName(defaultValue)), ValidValues = s_validValues,
        IsRequired = true,
    };

    public static NodeValueDef_Enum<T> Optional(T? defaultValue = null) => new()
    {
        TypedDefaultValue = defaultValue.HasValue ? new KdlString(KdlEnumUtils.GetName(defaultValue)) : null, ValidValues = s_validValues,
    };
}

public class NodeValueDef_Number<T> : NodeValueDef<KdlNumber<T>> where T : struct, INumber<T>
{
    public static NodeValueDef_Number<T> Required(T defaultValue = default) => new()
    {
        TypedDefaultValue = new KdlNumber<T>(defaultValue),
        IsRequired = true,
    };

    public static NodeValueDef_Number<T> Optional(T? defaultValue = null) => new()
    {
        TypedDefaultValue = defaultValue.HasValue ? new KdlNumber<T>(defaultValue.Value) : null,
    };
}

public class NodeValueDef_String : NodeValueDef<KdlString>
{
    public static NodeValueDef_String Required(string defaultValue = "") => new ()
    {
        TypedDefaultValue = new KdlString(defaultValue),
        IsRequired = true,
    };

    public static NodeValueDef_String Optional(string? defaultValue = null) => new()
    {
        TypedDefaultValue = defaultValue is not null ? new KdlString(defaultValue) : null,
    };
}

public class NodeValueDef_Path : NodeValueDef<KdlString>
{
    public static NodeValueDef_String Required(string defaultValue = "") => new()
    {
        TypedDefaultValue = new KdlString(defaultValue),
        IsRequired = true,
    };

    public static NodeValueDef_String Optional(string? defaultValue = null) => new()
    {
        TypedDefaultValue = defaultValue is not null ? new KdlString(defaultValue) : null,
    };
}
