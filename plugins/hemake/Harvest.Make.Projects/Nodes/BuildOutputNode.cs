// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class BuildOutputNodeTraits : NodeBaseTraits
{
    public override string Name => "build_output";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Path.Optional(".build"),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "bin_dir", NodeValueDef_String.Optional("${platform.name:lower}-${configuration.name:lower}/bin") },
        { "gen_dir", NodeValueDef_String.Optional("${platform.name:lower}-${configuration.name:lower}/generated") },
        { "lib_dir", NodeValueDef_String.Optional("${platform.name:lower}-${configuration.name:lower}/lib") },
        { "obj_dir", NodeValueDef_String.Optional("${platform.name:lower}-${configuration.name:lower}/obj") },
        { "install_dir", NodeValueDef_String.Optional("installs") },
        { "project_dir", NodeValueDef_String.Optional("projects") },
    };
}

public class BuildOutputNode(KdlNode node) : NodeBase<BuildOutputNodeTraits>(node)
{
    public string BasePath => GetPathValue(0);

    public string BinDir => Path.Combine(BasePath, GetStringValue("bin_dir"));
    public string GenDir => Path.Combine(BasePath, GetStringValue("gen_dir"));
    public string LibDir => Path.Combine(BasePath, GetStringValue("lib_dir"));
    public string ObjDir => Path.Combine(BasePath, GetStringValue("obj_dir"));
    public string InstallDir => Path.Combine(BasePath, GetStringValue("install_dir"));
    public string ProjectDir => Path.Combine(BasePath, GetStringValue("project_dir"));

    public override void ResolveDefaults(ProjectContext projectContext)
    {
        base.ResolveDefaults(projectContext);

        // Some of the default values include tokens that need to be resolved
        Node.Properties["bin_dir"] = MakeResolvedValue(projectContext, BinDir);
        Node.Properties["gen_dir"] = MakeResolvedValue(projectContext, GenDir);
        Node.Properties["lib_dir"] = MakeResolvedValue(projectContext, LibDir);
        Node.Properties["obj_dir"] = MakeResolvedValue(projectContext, ObjDir);
    }

    private KdlString MakeResolvedValue(ProjectContext projectContext, string input)
    {
        string resolved = projectContext.ProjectService.TokenReplacer.ReplaceTokens(projectContext, Node, input);
        return new KdlString(resolved);
    }
}
