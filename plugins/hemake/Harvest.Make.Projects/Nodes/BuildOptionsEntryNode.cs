// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class BuildOptionsEntryNodeTraits : NodeSetEntryBaseTraits<BuildOptionsNode>
{
    public override INode CreateNode(KdlNode node) => new BuildOptionsEntryNode(node);
}

public class BuildOptionsEntryNode(KdlNode node) : NodeSetEntryBase<BuildOptionsEntryNodeTraits, BuildOptionsNode>(node)
{
    public string OptionName => Node.Name;
}
