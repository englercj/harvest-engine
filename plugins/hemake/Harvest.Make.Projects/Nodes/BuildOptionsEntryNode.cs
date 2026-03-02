// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class BuildOptionsEntryNodeTraits : NodeSetEntryBaseTraits<BuildOptionsNode>
{
    public override INode CreateNode(KdlNode node) => new BuildOptionsEntryNode(node);
}

internal class BuildOptionsEntryNode(KdlNode node) : NodeSetEntryBase<BuildOptionsEntryNodeTraits, BuildOptionsNode>(node)
{
    public string OptionName => Node.Name;
}
