// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class OutputsEntryNodeTraits : NodeSetEntryBaseTraits<OutputsNode>
{
}

public class OutputsEntryNode(KdlNode node, INode? scope) : NodeSetEntryBase<OutputsEntryNodeTraits, OutputsNode>(node, scope)
{
    public string FilePath => ResolvePath(Node.Name);
}
