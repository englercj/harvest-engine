// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EWarningsLevel
{
    [KdlName("default")] Default,
    [KdlName("all")] All,
    [KdlName("extra")] Extra,
    [KdlName("on")] On,
    [KdlName("off")] Off,
}

public class WarningsNode(KdlNode node, INode? scope) : NodeSetBase<WarningsNode, WarningsEntryNode>(node, scope)
{
    public static string NodeName => "warnings";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "level", NodeValueDef_Enum<EWarningsLevel>.Optional(EWarningsLevel.Default) },
        { "fatal", NodeValueDef_Bool.Optional(false) },
    };

    public EWarningsLevel WarningsLevel => GetEnumValue<EWarningsLevel>(0);
    public bool AreAllWarningsFatal => GetBoolValue("fatal");
}
