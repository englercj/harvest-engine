// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class TagsNode(KdlNode node, INode? scope) : NodeSetBase<TagsNode, TagsEntryNode>(node, scope)
{
    public static string NodeName => "tags";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];
}
