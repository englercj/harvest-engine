// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class  InputsNodeTraits : NodeSetBaseTraits<InputsEntryNode>
{
    public override string Name => "inputs";

    public override IReadOnlyList<string> ValidScopes =>
    [
        BuildEventNode.NodeTraits.Name,
        BuildRuleNode.NodeTraits.Name,
    ];

    public override INode CreateNode(KdlNode node) => new InputsNode(node);
}

internal class InputsNode(KdlNode node) : NodeSetBase<InputsNodeTraits, InputsEntryNode>(node)
{
}
