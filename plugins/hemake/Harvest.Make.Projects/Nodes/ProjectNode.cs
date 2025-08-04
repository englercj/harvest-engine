// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ProjectNode(KdlNode node, INode? scope) : NodeBase<ProjectNode>(node, scope)
{
    public static string NodeName => "project";

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "start", NodeValueDef_String.Optional() },
    };

    public string ProjectName => GetStringValue(0);
    public string? StartupModule => TryGetStringValue("start");
}
