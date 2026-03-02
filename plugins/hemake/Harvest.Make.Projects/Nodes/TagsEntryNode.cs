// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class TagsEntryNodeTraits : NodeSetEntryBaseTraits<TagsNode>
{
    public override INode CreateNode(KdlNode node) => new TagsEntryNode(node);
}

internal class TagsEntryNode(KdlNode node) : NodeSetEntryBase<TagsEntryNodeTraits, TagsNode>(node)
{
    public string TagName => Node.Name;
}
