// Copyright Chad Engler

using Harvest.Kdl;

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
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "bin_dir", NodeValueDef_String.Optional("${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/bin") },
        { "gen_dir", NodeValueDef_String.Optional("${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/generated") },
        { "lib_dir", NodeValueDef_String.Optional("${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/lib") },
        { "obj_dir", NodeValueDef_String.Optional("${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/obj") },
    };

    public override INode CreateNode(KdlNode node) => new BuildOutputNode(node);
}

public class BuildOutputNode(KdlNode node) : NodeBase<BuildOutputNodeTraits>(node)
{
    private string BasePath => Path.GetDirectoryName(Node.SourceInfo.FilePath) ?? Directory.GetCurrentDirectory();

    public string BinDir => Path.GetFullPath(Path.Combine(BasePath, GetValue<string>("bin_dir")));
    public string GenDir => Path.GetFullPath(Path.Combine(BasePath, GetValue<string>("gen_dir")));
    public string LibDir => Path.GetFullPath(Path.Combine(BasePath, GetValue<string>("lib_dir")));
    public string ObjDir => Path.GetFullPath(Path.Combine(BasePath, GetValue<string>("obj_dir")));
}
