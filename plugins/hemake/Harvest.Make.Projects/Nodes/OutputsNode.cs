// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class OutputsNode(KdlNode node, INode? scope) : NodeSetBase<OutputsNode, OutputsEntryNode>(node, scope)
{
    public static string NodeName => "outputs";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        BuildEventNode.NodeName,
        BuildRuleNode.NodeName,
    ];
}
