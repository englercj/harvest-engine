// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class TagsEntryNodeTraits : NodeSetEntryBaseTraits<TagsNode>
{
}

public class TagsEntryNode(KdlNode node, INode? scope) : NodeSetEntryBase<TagsEntryNodeTraits, TagsNode>(node, scope)
{
    public string TagName => Node.Name;
}
