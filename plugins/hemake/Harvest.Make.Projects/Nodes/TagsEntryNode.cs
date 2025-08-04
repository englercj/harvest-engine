// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class TagsEntryNode(KdlNode node, INode? scope) : NodeBase<TagsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        TagsNode.NodeName,
    ];

    public string Tag => Node.Name;
}
