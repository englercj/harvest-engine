// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class OutputsEntryNodeTraits : NodeSetEntryBaseTraits<OutputsNode>
{
    public override INode CreateNode(KdlNode node) => new OutputsEntryNode(node);
}

internal class OutputsEntryNode(KdlNode node) : NodeSetEntryBase<OutputsEntryNodeTraits, OutputsNode>(node)
{
    public string FilePath => Node.Name;
}
