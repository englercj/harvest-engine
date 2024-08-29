// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class ArchiveNode(KdlNode node) : NodeBase(node), INode
{
    public const string NodeName = "archive";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        InstallNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlValue<KdlString>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "base", NodeKdlValue<KdlString>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? Url => GetStringValue(0);

    public string? BasePath => GetStringValue("base");
}
