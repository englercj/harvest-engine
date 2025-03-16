// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ExternalNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "external";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "warnings", NodeKdlEnum<EWarningsLevel>.Optional(EWarningsLevel.Default) },
        { "fatal", NodeKdlBool.Optional(false) },
        { "angle_brackets", NodeKdlBool.Optional(true) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EWarningsLevel WarningsLevel => GetEnumValue<EWarningsLevel>("warnings");
    public bool Fatal => GetBoolValue("fatal");
    public bool AngleBrackets => GetBoolValue("angle_brackets");
}
