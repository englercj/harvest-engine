// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class BuildRuleNodeTraits : NodeBaseTraits
{
    public override string Name => "build_rule";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        PluginNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "message", NodeValueDef_String.Optional() },
        { "link_output", NodeValueDef_Bool.Optional(true) },
    };

    public override INode CreateNode(KdlNode node) => new BuildRuleNode(node);
}

public class BuildRuleNode(KdlNode node) : NodeBase<BuildRuleNodeTraits>(node)
{
    public string RuleName => GetValue<string>(0);
    public string? Message => TryGetValue("message", out string? value) ? value : null;
    public bool LinkOutput => GetValue<bool>("link_output");
}
