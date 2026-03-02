// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class AuthorsNodeTraits : NodeSetBaseTraits<AuthorsEntryNode>
{
    public override string Name => "authors";

    public override IReadOnlyList<string> ValidScopes =>
    [
        PluginNode.NodeTraits.Name,
    ];

    public override INode CreateNode(KdlNode node) => new AuthorsNode(node);
}

internal class AuthorsNode(KdlNode node) : NodeSetBase<AuthorsNodeTraits, AuthorsEntryNode>(node)
{
}
