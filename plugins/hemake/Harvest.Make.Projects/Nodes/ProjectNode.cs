// Copyright Chad Engler

using Harvest.Kdl;
using System.Diagnostics;

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

    public override string? TryResolveToken(ProjectContext projectContext, KdlNode contextNode, string propertyName)
    {
        Debug.Assert(contextNode.Name == Name);

        ProjectNode project = new(contextNode);

        switch (propertyName)
        {
            case "name": return project.ProjectName;
        }

        return base.TryResolveToken(projectContext, contextNode, propertyName);
    }
}

public class ProjectNode(KdlNode node) : NodeBase<ProjectNodeTraits>(node)
{
    public string ProjectName => GetStringValue(0);
    public string? StartupModule => TryGetStringValue("start");
}
