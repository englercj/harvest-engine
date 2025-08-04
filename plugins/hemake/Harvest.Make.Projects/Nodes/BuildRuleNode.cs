// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class BuildRuleNode(KdlNode node, INode? scope) : NodeBase<BuildRuleNode>(node, scope)
{
    public static string NodeName => "build_rule";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        PluginNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "message", NodeValueDef_String.Optional() },
        { "link_output", NodeValueDef_Bool.Optional(true) },
    };

    public string RuleName => GetStringValue(0);
    public string? Message => TryGetStringValue("message");
    public bool LinkOutput => GetBoolValue("link_output");
}
