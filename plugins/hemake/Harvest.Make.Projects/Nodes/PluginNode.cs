// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class PluginNode(KdlNode node, INode? scope) : NodeBase<PluginNode>(node, scope)
{
    public static string NodeName => "plugin";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "version", NodeValueDef_String.Required() },
        { "license", NodeValueDef_String.Optional() },
    };

    public string PluginName => GetStringValue(0);
    public string Version => GetStringValue("version");
    public string? License => TryGetStringValue("license");
}
