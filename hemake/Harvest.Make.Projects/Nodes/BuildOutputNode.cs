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
        { "bin_dir", NodeValueDef_Path.Optional("${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/bin") },
        { "gen_dir", NodeValueDef_Path.Optional("${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/generated") },
        { "lib_dir", NodeValueDef_Path.Optional("${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/lib") },
        { "obj_dir", NodeValueDef_Path.Optional("${project.build_dir}/${platform.name:lower}-${configuration.name:lower}/obj") },
    };

    public override INode CreateNode(KdlNode node) => new BuildOutputNode(node);
}

public class BuildOutputNode(KdlNode node) : NodeBase<BuildOutputNodeTraits>(node)
{
    public string BinDir => GetValue<string>("bin_dir");
    public string GenDir => GetValue<string>("gen_dir");
    public string LibDir => GetValue<string>("lib_dir");
    public string ObjDir => GetValue<string>("obj_dir");
}
