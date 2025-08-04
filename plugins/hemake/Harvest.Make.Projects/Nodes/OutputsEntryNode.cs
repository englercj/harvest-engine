// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class OutputsEntryNode(KdlNode node, INode? scope) : NodeBase<OutputsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        OutputsNode.NodeName,
    ];

    public string FilePath => ResolvePath(Node.Name);
}
