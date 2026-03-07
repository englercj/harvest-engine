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

public class FloatingPointNodeTraits : NodeBaseTraits
{
    public override string Name => "floating_point";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EFloatingPointMode>.Required(EFloatingPointMode.Default),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "exceptions", NodeValueDef_Bool.Optional(false) },
    };

    public override INode CreateNode(KdlNode node) => new FloatingPointNode(node);
}

public class FloatingPointNode(KdlNode node) : NodeBase<FloatingPointNodeTraits>(node)
{
    public EFloatingPointMode Mode => GetEnumValue<EFloatingPointMode>(0);
    public bool AllowExceptions => GetValue<bool>("exceptions");
}
