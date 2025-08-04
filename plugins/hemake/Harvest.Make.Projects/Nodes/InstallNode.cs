// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InstallNode(KdlNode node, INode? scope) : NodeBase<InstallNode>(node, scope)
{
    public static string NodeName => "install";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        PluginNode.NodeName,
    ];
}
