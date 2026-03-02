// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class DefinesEntryNodeTraits : NodeSetEntryBaseTraits<DefinesNode>
{
    public override INode CreateNode(KdlNode node) => new DefinesEntryNode(node);
}

internal class DefinesEntryNode(KdlNode node) : NodeSetEntryBase<DefinesEntryNodeTraits, DefinesNode>(node)
{
    public string DefineName => Node.Name;
}
