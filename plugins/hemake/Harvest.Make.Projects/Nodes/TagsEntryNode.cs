// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class TagsEntryNodeTraits : NodeSetEntryBaseTraits<TagsNode>
{
}

public class TagsEntryNode(KdlNode node) : NodeSetEntryBase<TagsEntryNodeTraits, TagsNode>(node)
{
    public string TagName => Node.Name;
}
