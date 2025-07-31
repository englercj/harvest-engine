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

public class WarningsNode(KdlNode node, INode? scope) : NodeSetBase<WarningsEntryNode>(node, scope)
{
    public const string NodeName = "warnings";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "level", NodeKdlEnum<EWarningsLevel>.Optional(EWarningsLevel.Default) },
        { "fatal", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EWarningsLevel WarningsLevel => GetEnumValue<EWarningsLevel>(0);
    public bool AreAllWarningsFatal => GetBoolValue("fatal");
}
