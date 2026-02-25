// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class PublicNodeTraits : NodeBaseTraits
{
    public override string Name => "public";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
    ];

    public override INode CreateNode(KdlNode node) => new PublicNode(node);
}

public class PublicNode(KdlNode node) : NodeBase<PublicNodeTraits>(node)
{
}
