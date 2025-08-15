// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InstallNodeTraits : NodeBaseTraits
{
    public override string Name => "install";

    public override IReadOnlyList<string> ValidScopes =>
    [
        PluginNode.NodeTraits.Name,
    ];
}

public class InstallNode(KdlNode node, INode? scope) : NodeBase<InstallNodeTraits>(node, scope)
{
}
