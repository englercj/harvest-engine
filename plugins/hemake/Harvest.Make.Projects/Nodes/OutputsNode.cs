// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class OutputsNodeTraits : NodeSetBaseTraits<OutputsEntryNode>
{
    public override string Name => "outputs";

    public override IReadOnlyList<string> ValidScopes =>
    [
        BuildEventNode.NodeTraits.Name,
        BuildRuleNode.NodeTraits.Name,
    ];

    public override INode CreateNode(KdlNode node) => new OutputsNode(node);
}

internal class OutputsNode(KdlNode node) : NodeSetBase<OutputsNodeTraits, OutputsEntryNode>(node)
{
}
