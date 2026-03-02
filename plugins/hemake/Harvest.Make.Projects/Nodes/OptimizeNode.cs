// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EOptimizationLevel
{
    [KdlName("default")] Default,
    [KdlName("debug")] Debug,
    [KdlName("off")] Off,
    [KdlName("on")] On,
    [KdlName("size")] Size,
    [KdlName("speed")] Speed,
}

public enum ELinkTimeOptimizationLevel
{
    [KdlName("default")] Default,
    [KdlName("off")] Off,
    [KdlName("on")] On,
}

public enum EInliningLevel
{
    [KdlName("default")] Default,
    [KdlName("off")] Off,
    [KdlName("explicit")] Explicit,
    [KdlName("on")] On,
}

internal class OptimizeNodeTraits : NodeBaseTraits
{
    public override string Name => "optimize";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EOptimizationLevel>.Required(EOptimizationLevel.Default),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "lto", NodeValueDef_Enum<ELinkTimeOptimizationLevel>.Optional(ELinkTimeOptimizationLevel.Default) },
        { "inlining", NodeValueDef_Enum<EInliningLevel>.Optional(EInliningLevel.Default) },
        { "function_level_linking", NodeValueDef_Bool.Optional() },
        { "string_pooling", NodeValueDef_Bool.Optional() },
        { "intrinsics", NodeValueDef_Bool.Optional() },
        { "just_my_code", NodeValueDef_Bool.Optional(true) },
    };

    public override INode CreateNode(KdlNode node) => new OptimizeNode(node);
}

internal class OptimizeNode(KdlNode node) : NodeBase<OptimizeNodeTraits>(node)
{
    public EOptimizationLevel OptimizationLevel => GetEnumValue<EOptimizationLevel>(0);
    public ELinkTimeOptimizationLevel LinkTimeOptimizationLevel => GetEnumValue<ELinkTimeOptimizationLevel>("lto");
    public EInliningLevel InliningLevel => GetEnumValue<EInliningLevel>("inlining");
    public bool? FunctionLevelLinking => TryGetValue("function_level_linking", out bool value) ? value : null;
    public bool? StringPooling => TryGetValue("string_pooling", out bool value) ? value : null;
    public bool? Intrinsics => TryGetValue("intrinsics", out bool value) ? value : null;
    public bool JustMyCode => GetValue<bool>("just_my_code");
}
