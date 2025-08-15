// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class AuthorsNodeTraits : NodeSetBaseTraits<AuthorsEntryNode>
{
    public override string Name => "authors";

    public override IReadOnlyList<string> ValidScopes =>
    [
        PluginNode.NodeTraits.Name,
    ];
}

public class AuthorsNode(KdlNode node, INode? scope) : NodeSetBase<AuthorsNodeTraits, AuthorsEntryNode>(node, scope)
{
}
