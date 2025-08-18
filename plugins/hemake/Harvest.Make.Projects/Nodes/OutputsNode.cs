// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class OutputsNodeTraits : NodeSetBaseTraits<OutputsEntryNode>
{
    public override string Name => "outputs";

    public override IReadOnlyList<string> ValidScopes =>
    [
        BuildEventNode.NodeTraits.Name,
        BuildRuleNode.NodeTraits.Name,
    ];
}

public class OutputsNode(KdlNode node) : NodeSetBase<OutputsNodeTraits, OutputsEntryNode>(node)
{
}
