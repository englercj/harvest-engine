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

public class FloatingPointNode(KdlNode node, INode? scope) : NodeBase<FloatingPointNode>(node, scope)
{
    public static string NodeName => "floating_point";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_Enum<EFloatingPointMode>.Required(EFloatingPointMode.Default),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "exceptions", NodeValueDef_Bool.Optional(false) },
    };

    public EFloatingPointMode Mode => GetEnumValue<EFloatingPointMode>(0);
    public bool AllowExceptions => GetBoolValue("exceptions");
}
