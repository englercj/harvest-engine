// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InputsEntryNodeTraits : NodeSetEntryBaseTraits<InputsNode>
{
}

public class InputsEntryNode(KdlNode node) : NodeSetEntryBase<InputsEntryNodeTraits, InputsNode>(node)
{
    public string FilePath => ResolvePath(Node.Name);
}
