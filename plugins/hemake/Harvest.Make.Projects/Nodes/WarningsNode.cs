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

public class WarningsNodeTraits : NodeSetBaseTraits<WarningsEntryNode>
{
    public override string Name => "warnings";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "level", NodeValueDef_Enum<EWarningsLevel>.Optional(EWarningsLevel.Default) },
        { "fatal", NodeValueDef_Bool.Optional(false) },
    };
}

public class WarningsNode(KdlNode node, INode? scope) : NodeSetBase<WarningsNodeTraits, WarningsEntryNode>(node, scope)
{
    public EWarningsLevel WarningsLevel => GetEnumValue<EWarningsLevel>(0);
    public bool AreAllWarningsFatal => GetBoolValue("fatal");
}
