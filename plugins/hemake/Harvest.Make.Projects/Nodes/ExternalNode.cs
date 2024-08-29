// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class ExternalNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "external";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "warnings", NodeKdlEnum<EWarningsMode>.Optional },
        { "fatal", NodeKdlValue<KdlBool>.Optional },
        { "angle_brackets", NodeKdlValue<KdlBool>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EWarningsMode WarningsMode => GetEnumValue("warnings", EWarningsMode.Default);

    public bool Fatal => GetBoolValue("fatal") ?? false;
    public bool AngleBrackets => GetBoolValue("angle_brackets") ?? false;
}
