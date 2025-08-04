// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InputsNode(KdlNode node, INode? scope) : NodeSetBase<InputsNode, InputsEntryNode>(node, scope)
{
    public static string NodeName => "inputs";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        BuildEventNode.NodeName,
        BuildRuleNode.NodeName,
    ];
}
