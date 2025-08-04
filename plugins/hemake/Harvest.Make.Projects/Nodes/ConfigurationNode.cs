// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ConfigurationNode(KdlNode node, INode? scope) : NodeBase<ConfigurationNode>(node, scope)
{
    public static string NodeName => "configuration";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public string ConfigName => GetStringValue(0);
}
