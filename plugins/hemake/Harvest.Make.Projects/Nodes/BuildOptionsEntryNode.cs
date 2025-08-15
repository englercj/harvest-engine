// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class BuildOptionsEntryNodeTraits : NodeSetEntryBaseTraits<BuildOptionsNode>
{
}

public class BuildOptionsEntryNode(KdlNode node, INode? scope) : NodeSetEntryBase<BuildOptionsEntryNodeTraits, BuildOptionsNode>(node, scope)
{
    public string OptionName => Node.Name;
}
