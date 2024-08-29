// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;
using System.Numerics;

namespace Harvest.Make.Projects.Nodes;

public enum EOptionType
{
    [KdlName("bool")] Bool,
    [KdlName("number")] Number,
    [KdlName("string")] String,
}

public class OptionNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "option";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlValue<KdlString>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "default", NodeKdlValue<KdlString>.Optional },
        { "env", NodeKdlValue<KdlString>.Optional },
        { "help", NodeKdlValue<KdlString>.Optional },
        { "type", NodeKdlEnum<EOptionType>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? HelpText => GetStringValue("help");
    public EOptionType OptionType => GetEnumValue("type", EOptionType.String);
    public string? EnvVarName => GetStringValue("env");

    public bool? GetDefaultBool() => GetBoolValue("default");
    public T? GetDefaultNumber<T>() where T : struct, INumber<T> => GetNumberValue<T>("default");
    public string? GetDefaultString() => GetStringValue("default");
}
