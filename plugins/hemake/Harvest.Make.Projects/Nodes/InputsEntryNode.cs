// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InputsEntryNode(KdlNode node, INode? scope) : NodeBase<InputsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        InputsNode.NodeName,
    ];

    public string FilePath => ResolvePath(Node.Name);
}
