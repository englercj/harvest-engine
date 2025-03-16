// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EFloatingPointMode
{
    [KdlName("default")] Default,
    [KdlName("fast")] Fast,
    [KdlName("precise")] Precise,
    [KdlName("strict")] Strict,
}

public class FloatingPointNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "floating_point";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EFloatingPointMode>.Required(EFloatingPointMode.Default),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "exceptions", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EFloatingPointMode Mode => GetEnumValue<EFloatingPointMode>(0);
    public bool AllowExceptions => GetBoolValue("exceptions");
}
