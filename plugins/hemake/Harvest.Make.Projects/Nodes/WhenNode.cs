// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EWhenMode
{
    [KdlName("and")] And,
    [KdlName("or")] Or,
    [KdlName("xor")] Xor,
}

public class WhenNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "when";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        InstallNode.NodeName,
        ModuleNode.NodeName,
        PluginNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EWhenMode>.Optional,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "fatal", NodeKdlValue<KdlBool>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EWhenMode Mode => GetEnumValue(0, EWhenMode.And);
    public string? Arch => GetStringValue("arch");
    public string? Configuration => GetStringValue("configuration");
    public string? Host => GetStringValue("host");
    public string? Language => GetStringValue("language");
    public string? Option => GetStringValue("option");
    public string? Platform => GetStringValue("platform");
    public string? System => GetStringValue("system");
    public string? Tags => GetStringValue("tags");
    public string? Toolset => GetStringValue("toolset");
}
