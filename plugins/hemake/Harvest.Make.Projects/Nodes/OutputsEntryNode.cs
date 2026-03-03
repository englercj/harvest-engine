// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class OutputsEntryNodeTraits : NodeSetEntryBaseTraits<OutputsNode>
{
    public override INode CreateNode(KdlNode node) => new OutputsEntryNode(node);
}

public class OutputsEntryNode(KdlNode node) : NodeSetEntryBase<OutputsEntryNodeTraits, OutputsNode>(node)
{
    public string FilePath => ResolveSinglePath(Node.Name);
}
