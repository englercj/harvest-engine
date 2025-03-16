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

public class SymbolsNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "symbols";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<ESymbolsMode>.Required(ESymbolsMode.Default),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "embed", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public ESymbolsMode SymbolsMode => GetEnumValue<ESymbolsMode>(0);
    public bool Embed => GetBoolValue("embed");
}
