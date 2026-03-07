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
        { "build_dir", NodeValueDef_Path.Optional(".build") },
        { "installs_dir", NodeValueDef_Path.Optional("${project.build_dir}/installs") },
        { "projects_dir", NodeValueDef_Path.Optional("${project.build_dir}/projects") },
    };

    public override INode CreateNode(KdlNode node) => new ProjectNode(node);

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
    public string ProjectName => GetValue<string>(0);
    public string? StartupModule => TryGetValue("start", out string? value) ? value : null;
    public string BuildDir => GetValue<string>("build_dir");
    public string InstallsDir => GetValue<string>("installs_dir");
    public string ProjectsDir => GetValue<string>("projects_dir");
}
