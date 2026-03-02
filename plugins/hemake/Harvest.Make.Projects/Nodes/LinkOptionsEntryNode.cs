// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LinkOptionsEntryNodeTraits : NodeSetEntryBaseTraits<LinkOptionsNode>
{
    public override INode CreateNode(KdlNode node) => new LinkOptionsEntryNode(node);
}

public class LinkOptionsEntryNode(KdlNode node) : NodeSetEntryBase<LinkOptionsEntryNodeTraits, LinkOptionsNode>(node)
{
    public string OptionName => Node.Name;
}
