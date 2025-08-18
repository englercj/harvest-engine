// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class OutputsEntryNodeTraits : NodeSetEntryBaseTraits<OutputsNode>
{
}

public class OutputsEntryNode(KdlNode node) : NodeSetEntryBase<OutputsEntryNodeTraits, OutputsNode>(node)
{
    public string FilePath => ResolvePath(Node.Name);
}
