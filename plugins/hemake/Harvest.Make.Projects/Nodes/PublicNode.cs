// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class PublicNode(KdlNode node, INode? scope) : NodeBase<PublicNode>(node, scope)
{
    public static string NodeName => "public";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
    ];
}
