// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ProjectNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "project";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlString.Required(),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "start", NodeKdlString.Optional() },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string ProjectName => GetStringValue(0);
    public string? StartupModule => TryGetStringValue("start");
}
