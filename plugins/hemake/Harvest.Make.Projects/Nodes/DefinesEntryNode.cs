// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class DefinesEntryNodeTraits : NodeSetEntryBaseTraits<DefinesNode>
{
    public override INode CreateNode(KdlNode node) => new DefinesEntryNode(node);
}

public class DefinesEntryNode(KdlNode node) : NodeSetEntryBase<DefinesEntryNodeTraits, DefinesNode>(node)
{
    public string DefineName => Node.Name;
}
