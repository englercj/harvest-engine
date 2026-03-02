// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class InputsEntryNodeTraits : NodeSetEntryBaseTraits<InputsNode>
{
    public override INode CreateNode(KdlNode node) => new InputsEntryNode(node);
}

internal class InputsEntryNode(KdlNode node) : NodeSetEntryBase<InputsEntryNodeTraits, InputsNode>(node)
{
    public string FilePath => Node.Name;
}
