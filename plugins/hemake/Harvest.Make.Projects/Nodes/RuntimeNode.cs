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

public class RuntimeNodeTraits : NodeBaseTraits
{
    public override string Name => "runtime";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<ERuntime>.Required(ERuntime.Default),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "static", NodeValueDef_Bool.Optional(false) },
    };

    public override INode CreateNode(KdlNode node) => new RuntimeNode(node);
}

public class RuntimeNode(KdlNode node) : NodeBase<RuntimeNodeTraits>(node)
{
    public ERuntime Runtime => GetEnumValue<ERuntime>(0);
    public bool StaticLink => GetValue<bool>("static");
}
