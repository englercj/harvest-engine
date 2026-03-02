// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class PublicNodeTraits : NodeBaseTraits
{
    public override string Name => "public";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
    ];

    public override INode CreateNode(KdlNode node) => new PublicNode(node);
}

internal class PublicNode(KdlNode node) : NodeBase<PublicNodeTraits>(node)
{
}
