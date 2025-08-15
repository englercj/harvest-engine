// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class DefinesEntryNodeTraits : NodeSetEntryBaseTraits<DefinesNode>
{
}

public class DefinesEntryNode(KdlNode node, INode? scope) : NodeSetEntryBase<DefinesEntryNodeTraits, DefinesNode>(node, scope)
{
    public string DefineName => Node.Name;
}
