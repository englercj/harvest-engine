// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class DefinesEntryNode(KdlNode node, INode? scope) : NodeBase<DefinesEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        DefinesNode.NodeName,
    ];

    public string Define => Node.Name;
}
