// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class LinkOptionsEntryNodeTraits : NodeSetEntryBaseTraits<LinkOptionsNode>
{
    public override INode CreateNode(KdlNode node) => new LinkOptionsEntryNode(node);
}

internal class LinkOptionsEntryNode(KdlNode node) : NodeSetEntryBase<LinkOptionsEntryNodeTraits, LinkOptionsNode>(node)
{
    public string OptionName => Node.Name;
}
