// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class TagsNodeTraits : NodeSetBaseTraits<TagsEntryNode>
{
    public override string Name => "tags";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];
}

public class TagsNode(KdlNode node, INode? scope) : NodeSetBase<TagsNodeTraits, TagsEntryNode>(node, scope)
{
}
