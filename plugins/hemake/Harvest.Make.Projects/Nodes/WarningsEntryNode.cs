// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EWarningAction
{
    [KdlName("enable")] Enable,
    [KdlName("disable")] Disable,
}

public class WarningsEntryNode(KdlNode node, INode? scope) : NodeBase<WarningsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        WarningsNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_Enum<EWarningAction>.Optional(EWarningAction.Enable),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "fatal", NodeValueDef_Bool.Optional(false) },
    };

    public string WarningName => Node.Name;
    public bool IsEnabled => GetEnumValue<EWarningAction>(0) == EWarningAction.Enable;
    public bool IsFatal => GetBoolValue("fatal");
}
