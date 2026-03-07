// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InputsEntryNodeTraits : NodeSetEntryBaseTraits<InputsNode>
{
    public override INode CreateNode(KdlNode node) => new InputsEntryNode(node);
}

public class InputsEntryNode(KdlNode node) : NodeSetEntryBase<InputsEntryNodeTraits, InputsNode>(node)
{
    public string FilePath => ResolveSinglePath(Node.Name);
}
