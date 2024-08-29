// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class ProjectNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "project";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlValue<KdlString>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "start", NodeKdlValue<KdlString>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? ProjectName => GetStringValue(0);

    public string? StartupModule => GetStringValue("start");
}
