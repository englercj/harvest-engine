// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LinkOptionsEntryNodeTraits : NodeSetEntryBaseTraits<LinkOptionsNode>
{
}

public class LinkOptionsEntryNode(KdlNode node, INode? scope) : NodeSetEntryBase<LinkOptionsEntryNodeTraits, LinkOptionsNode>(node, scope)
{
    public string OptionName => Node.Name;
}
