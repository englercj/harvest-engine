// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EWarningAction
{
    [KdlName("enable")] Enable,
    [KdlName("disable")] Disable,
}

internal class WarningsEntryNodeTraits : NodeSetEntryBaseTraits<WarningsNode>
{
    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EWarningAction>.Optional(EWarningAction.Enable),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "fatal", NodeValueDef_Bool.Optional(false) },
    };

    public override INode CreateNode(KdlNode node) => new WarningsEntryNode(node);
}

internal class WarningsEntryNode(KdlNode node) : NodeSetEntryBase<WarningsEntryNodeTraits, WarningsNode>(node)
{
    public string WarningName => Node.Name;
    public bool IsEnabled => GetEnumValue<EWarningAction>(0) == EWarningAction.Enable;
    public bool IsFatal => GetValue<bool>("fatal");
}
