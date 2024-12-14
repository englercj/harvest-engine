// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;
using System.Numerics;

namespace Harvest.Make.Projects.Nodes;

public enum EOptionType
{
    [KdlName("bool")] Bool,
    [KdlName("int")] Int,
    [KdlName("uint")] UInt,
    [KdlName("float")] Float,
    [KdlName("string")] String,
}

public class OptionNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "option";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlString.Required(),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "default", NodeKdlString.Optional() },
        { "env", NodeKdlString.Optional() },
        { "help", NodeKdlString.Optional() },
        { "type", NodeKdlEnum<EOptionType>.Optional(EOptionType.String) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string OptionName => GetStringValue(0);
    public EOptionType OptionType => GetEnumValue<EOptionType>("type");
    public string? HelpText => TryGetStringValue("help");
    public string? EnvVarName => TryGetStringValue("env");

    public bool? GetDefaultBool() => OptionType == EOptionType.Bool ? TryGetBoolValue("default") : null;
    public string? GetDefaultString() => OptionType == EOptionType.String ? TryGetStringValue("default") : null;
    public T? GetDefaultNumber<T>() where T : struct, INumber<T>
    {
        string key = "default";

        if (OptionType != EOptionType.Int && OptionType != EOptionType.UInt && OptionType != EOptionType.Float)
        {
            return null;
        }

        if (Node.Properties.TryGetValue(key, out KdlValue? value))
        {
            return value switch
            {
                KdlNumber<byte> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<ushort> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<uint> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<ulong> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<sbyte> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<short> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<int> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<long> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<nint> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<nuint> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<float> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<double> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                KdlNumber<decimal> n => (T)Convert.ChangeType(n.Value, typeof(T)),
                _ => null,
            };
        }

        if (Properties.TryGetValue(key, out NodeKdlValue? valueDef))
        {
            if (valueDef.DefaultValue is string str)
            {
                switch (OptionType)
                {
                    case EOptionType.Int: return (T)Convert.ChangeType(Convert.ToInt64(str), typeof(T));
                    case EOptionType.UInt: return (T)Convert.ChangeType(Convert.ToUInt64(str), typeof(T));
                    case EOptionType.Float: return (T)Convert.ChangeType(Convert.ToDouble(str), typeof(T));
                }
            }
        }

        return null;
    }

    protected override NodeValidationResult ValidateProperties()
    {
        NodeValidationResult vr = base.ValidateProperties();
        if (!vr.IsValid)
        {
            return vr;
        }

        if (!Node.Properties.TryGetValue("default", out KdlValue? value))
            return NodeValidationResult.Valid;

        switch (OptionType)
        {
            case EOptionType.Bool:
            {
                if (value is not KdlBool)
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be a boolean.");
                }
                return NodeValidationResult.Valid;
            }
            case EOptionType.Int:
            {
                if (!ReflectionUtils.IsInstanceOfGenericType(value.GetType(), typeof(KdlNumber<>)) || !ReflectionUtils.IsTypeIntegral(value.GetType().GetGenericArguments()[0]))
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be a integer.");
                }
                return NodeValidationResult.Valid;
            }
            case EOptionType.UInt:
            {
                if (!ReflectionUtils.IsInstanceOfGenericType(value.GetType(), typeof(KdlNumber<>)) || !ReflectionUtils.IsTypeIntegral(value.GetType().GetGenericArguments()[0]))
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be an unsigned integer.");
                }
                if (value is KdlNumber<sbyte> b && b.Value < 0)
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be an unsigned integer.");
                }
                if (value is KdlNumber<int> i && i.Value < 0)
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be an unsigned integer.");
                }
                if (value is KdlNumber<long> l && l.Value < 0)
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be an unsigned integer.");
                }
                if (value is KdlNumber<ulong> u && u.Value < 0)
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be an unsigned integer.");
                }
                return NodeValidationResult.Valid;
            }
            case EOptionType.Float:
            {
                if (!ReflectionUtils.IsInstanceOfGenericType(value.GetType(), typeof(KdlNumber<>)) || !ReflectionUtils.IsTypeFloatingPoint(value.GetType().GetGenericArguments()[0]))
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be a floating-point number.");
                }
                return NodeValidationResult.Valid;
            }
            case EOptionType.String:
            {
                if (value is not KdlString)
                {
                    return NodeValidationResult.Error($"Default value for option {OptionName} must be a string.");
                }
                return NodeValidationResult.Valid;
            }
        }

        return NodeValidationResult.Error("Unknown option type");
    }
}
