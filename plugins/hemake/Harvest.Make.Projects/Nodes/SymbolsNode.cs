// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum ESymbolsMode
{
    [KdlName("default")] Default,
    [KdlName("on")] On,
    [KdlName("off")] Off,
}

public class SymbolsNodeTraits : NodeBaseTraits
{
    public override string Name => "symbols";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<ESymbolsMode>.Required(ESymbolsMode.Default),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "embed", NodeValueDef_Bool.Optional(false) },
    };
}

public class SymbolsNode(KdlNode node) : NodeBase<SymbolsNodeTraits>(node)
{
    public ESymbolsMode SymbolsMode => GetEnumValue<ESymbolsMode>(0);
    public bool Embed => GetBoolValue("embed");
}
