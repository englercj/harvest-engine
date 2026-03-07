// Copyright Chad Engler

using Harvest.Common.Extensions;
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

public class OptionNodeTraits : NodeBaseTraits
{
    public override string Name => "option";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "default", NodeValueDef_String.Optional() },
        { "env", NodeValueDef_String.Optional() },
        { "help", NodeValueDef_String.Optional() },
        { "type", NodeValueDef_Enum<EOptionType>.Optional(EOptionType.String) },
    };

    public override INode CreateNode(KdlNode node) => new OptionNode(node);

    protected override void ValidateProperties(KdlNode node)
    {
        base.ValidateProperties(node);

        if (!node.Properties.TryGetValue("default", out KdlValue? value))
        {
            return; // No default value to validate.
        }

        OptionNode option = new(node);

        switch (option.OptionType)
        {
            case EOptionType.Bool:
            {
                if (value is not KdlBool)
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be a boolean.");
                }
                return;
            }
            case EOptionType.Int:
            {
                if (!value.GetType().IsInstanceOfGenericType(typeof(KdlNumber<>)) || !value.GetType().GetGenericArguments()[0].IsTypeIntegral())
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be a integer.");
                }
                return;
            }
            case EOptionType.UInt:
            {
                if (!value.GetType().IsInstanceOfGenericType(typeof(KdlNumber<>)) || !value.GetType().GetGenericArguments()[0].IsTypeUnsignedIntegral())
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be an unsigned integer.");
                }
                if (value is KdlNumber<sbyte> b && b.Value < 0)
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be an unsigned integer.");
                }
                if (value is KdlNumber<int> i && i.Value < 0)
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be an unsigned integer.");
                }
                if (value is KdlNumber<long> l && l.Value < 0)
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be an unsigned integer.");
                }
                if (value is KdlNumber<ulong> u && u.Value < 0)
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be an unsigned integer.");
                }
                return;
            }
            case EOptionType.Float:
            {
                if (!value.GetType().IsInstanceOfGenericType(typeof(KdlNumber<>)) || !value.GetType().GetGenericArguments()[0].IsTypeFloatingPoint())
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be a floating-point number.");
                }
                return;
            }
            case EOptionType.String:
            {
                if (value is not KdlString)
                {
                    throw new NodeParseException(node, $"Default value for option {option.OptionName} must be a string.");
                }
                return;
            }
        }

        throw new NodeParseException(node, "Unknown option type");
    }
}

public class OptionNode(KdlNode node) : NodeBase<OptionNodeTraits>(node)
{
    public string OptionName => GetValue<string>(0);
    public EOptionType OptionType => GetEnumValue<EOptionType>("type");
    public string? HelpText => TryGetValue("help", out string? value) ? value : null;
    public string? EnvVarName => TryGetValue("env", out string? value) ? value : null;

    public bool? GetDefaultBool() => OptionType == EOptionType.Bool ? (TryGetValue("default", out bool value) ? value : null) : null;
    public string? GetDefaultString() => OptionType == EOptionType.String ? (TryGetValue("default", out string? value) ? value : null) : null;
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

        if (Traits.PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
        {
            if (valueDef.DefaultValue is KdlString str && str.Value is not null)
            {
                switch (OptionType)
                {
                    case EOptionType.Int: return (T)Convert.ChangeType(Convert.ToInt64(str.Value), typeof(T));
                    case EOptionType.UInt: return (T)Convert.ChangeType(Convert.ToUInt64(str.Value), typeof(T));
                    case EOptionType.Float: return (T)Convert.ChangeType(Convert.ToDouble(str.Value), typeof(T));
                }
            }
        }

        return null;
    }
}
