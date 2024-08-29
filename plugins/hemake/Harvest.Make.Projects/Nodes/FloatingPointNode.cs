// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EFloatingPointMode
{
    [KdlName("default")] Default,
    [KdlName("fast")] Fast,
    [KdlName("strict")] Strict,
}

public class FloatingPointNode(KdlNode node) : NodeSetBase<DefinesEntryNode>(node)
{
    public const string NodeName = "floating_point";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EFloatingPointMode>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "exceptions", NodeKdlValue<KdlBool>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EFloatingPointMode Mode => GetEnumValue(0, EFloatingPointMode.Default);
    public bool AllowExceptions => GetBoolValue("exceptions") ?? false;
}
