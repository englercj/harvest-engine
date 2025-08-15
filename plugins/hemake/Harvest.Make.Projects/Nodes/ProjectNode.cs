// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ProjectNodeTraits : NodeBaseTraits
{
    public override string Name => "project";

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "start", NodeValueDef_String.Optional() },
    };
}

public class ProjectNode(KdlNode node, INode? scope) : NodeBase<ProjectNodeTraits>(node, scope)
{
    public string ProjectName => GetStringValue(0);
    public string? StartupModule => TryGetStringValue("start");
}
