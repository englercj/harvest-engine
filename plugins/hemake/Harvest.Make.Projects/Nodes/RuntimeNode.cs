// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum ERuntime
{
    [KdlName("default")] Default,
    [KdlName("debug")] Debug,
    [KdlName("release")] Release,
}

public class RuntimeNode(KdlNode node, INode? scope) : NodeBase<RuntimeNode>(node, scope)
{
    public static string NodeName => "runtime";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_Enum<ERuntime>.Required(ERuntime.Default),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "static", NodeValueDef_Bool.Optional(false) },
    };

    public ERuntime Runtime => GetEnumValue<ERuntime>(0);
    public bool StaticLink => GetBoolValue("static");
}
