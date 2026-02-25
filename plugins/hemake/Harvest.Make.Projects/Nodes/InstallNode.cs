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

    public override INode CreateNode(KdlNode node) => new InstallNode(node);
}

public class InstallNode(KdlNode node) : NodeBase<InstallNodeTraits>(node)
{
}
