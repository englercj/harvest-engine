// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EWarningAction
{
    [KdlName("enable")] Enable,
    [KdlName("disable")] Disable,
}

public class WarningsEntryNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        WarningsNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EWarningAction>.Optional(EWarningAction.Enable),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "fatal", NodeKdlBool.Optional(false) },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string WarningName => Node.Name;
    public bool IsEnabled => GetEnumValue<EWarningAction>(0) == EWarningAction.Enable;
    public bool IsFatal => GetBoolValue("fatal");
}
