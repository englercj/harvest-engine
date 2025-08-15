// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public abstract class NodeSetEntryBaseTraits<TParent> : NodeBaseTraits
    where TParent : INode
{
    public override IReadOnlyList<string> ValidScopes =>
    [
        TParent.NodeTraits.Name,
    ];
}

public class NodeSetEntryBase<TTraits, TParent>(KdlNode node, INode? scope) : NodeBase<TTraits>(node, scope)
    where TTraits : NodeSetEntryBaseTraits<TParent>, new()
    where TParent : INode
{
}
