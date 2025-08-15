// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InputsEntryNodeTraits : NodeSetEntryBaseTraits<InputsNode>
{
}

public class InputsEntryNode(KdlNode node, INode? scope) : NodeSetEntryBase<InputsEntryNodeTraits, InputsNode>(node, scope)
{
    public string FilePath => ResolvePath(Node.Name);
}
