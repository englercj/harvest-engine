// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class CommandsEntryNode(KdlNode node) : NodeBase(node)
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        CommandsNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? Command => Node.Name;
}
