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

public class SymbolsNode(KdlNode node, INode? scope) : NodeBase<SymbolsNode>(node, scope)
{
    public static string NodeName => "symbols";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_Enum<ESymbolsMode>.Required(ESymbolsMode.Default),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "embed", NodeValueDef_Bool.Optional(false) },
    };

    public ESymbolsMode SymbolsMode => GetEnumValue<ESymbolsMode>(0);
    public bool Embed => GetBoolValue("embed");
}
