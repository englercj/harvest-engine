// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class BuildOptionsEntryNode(KdlNode node, INode? scope) : NodeBase<BuildOptionsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        BuildOptionsNode.NodeName,
    ];

    public string Option => Node.Name;
}
