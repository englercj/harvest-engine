// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class  InputsNodeTraits : NodeSetBaseTraits<InputsEntryNode>
{
    public override string Name => "inputs";

    public override IReadOnlyList<string> ValidScopes =>
    [
        BuildEventNode.NodeTraits.Name,
        BuildRuleNode.NodeTraits.Name,
    ];
}

public class InputsNode(KdlNode node, INode? scope) : NodeSetBase<InputsNodeTraits, InputsEntryNode>(node, scope)
{
}
