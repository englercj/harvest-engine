// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EOptimizationLevel
{
    [KdlName("default")] Default,
    [KdlName("debug")] Debug,
    [KdlName("off")] Off,
    [KdlName("balanced")] Balanced,
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

public class OptimizeNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "optimize";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EOptimizationLevel>.Required(EOptimizationLevel.Default),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "lto", NodeKdlEnum<ELinkTimeOptimizationLevel>.Optional(ELinkTimeOptimizationLevel.Default) },
        { "inlining", NodeKdlEnum<EInliningLevel>.Optional(EInliningLevel.Default) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EOptimizationLevel OptimizationLevel => GetEnumValue<EOptimizationLevel>(0);
    public ELinkTimeOptimizationLevel LinkTimeOptimizationLevel => GetEnumValue<ELinkTimeOptimizationLevel>("lto");
    public EInliningLevel InliningLevel => GetEnumValue<EInliningLevel>("inlining");
}
