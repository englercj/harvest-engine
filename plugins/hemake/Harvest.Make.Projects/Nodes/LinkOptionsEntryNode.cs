// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LinkOptionsEntryNode(KdlNode node, INode? scope) : NodeBase<LinkOptionsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        LinkOptionsNode.NodeName,
    ];

    public string Option => Node.Name;
}
