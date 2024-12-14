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
        { "warnings", NodeKdlEnum<EWarningsMode>.Optional(EWarningsMode.Default) },
        { "fatal", NodeKdlBool.Optional(false) },
        { "angle_brackets", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EWarningsMode WarningsMode => GetEnumValue<EWarningsMode>("warnings");
    public bool Fatal => GetBoolValue("fatal");
    public bool AngleBrackets => GetBoolValue("angle_brackets");
}
