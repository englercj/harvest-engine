// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class TagsEntryNodeTraits : NodeSetEntryBaseTraits<TagsNode>
{
    public override INode CreateNode(KdlNode node) => new TagsEntryNode(node);
}

public class TagsEntryNode(KdlNode node) : NodeSetEntryBase<TagsEntryNodeTraits, TagsNode>(node)
{
    public string TagName => Node.Name;
}
