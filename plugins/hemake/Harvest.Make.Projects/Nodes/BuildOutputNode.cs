// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class BuildOutputNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "build_output";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlPath.Required("build"),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "bin_dir", NodeKdlPath.Optional("${platform.name:lower}-${configuration.name:lower}/bin") },
        { "gen_dir", NodeKdlPath.Optional("${platform.name:lower}-${configuration.name:lower}/generated/${module.name}") },
        { "lib_dir", NodeKdlPath.Optional("${platform.name:lower}-${configuration.name:lower}/lib/${module.name}") },
        { "obj_dir", NodeKdlPath.Optional("${platform.name:lower}-${configuration.name:lower}/obj/${module.name}") },
        { "plugin_dir", NodeKdlPath.Optional("plugins") },
        { "project_dir", NodeKdlPath.Optional("projects") },
        { "target_name", NodeKdlString.Optional() },
        { "target_extension", NodeKdlString.Optional() },
        { "make_import_lib", NodeKdlBool.Optional(true) },
        { "make_exe_manifest", NodeKdlBool.Optional(true) },
        { "make_map_file", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string BasePath => GetPathValue(0);

    public string BinDir => GetPathValue("bin_dir");
    public string GenDir => GetPathValue("gen_dir");
    public string LibDir => GetPathValue("lib_dir");
    public string ObjDir => GetPathValue("obj_dir");
    public string PluginDir => GetPathValue("plugin_dir");
    public string ProjectDir => GetPathValue("project_dir");
    public string? TargetName => TryGetStringValue("target_name");
    public string? TargetExtension => TryGetStringValue("target_extension");
    public bool MakeImportLib => GetBoolValue("make_import_lib");
    public bool MakeExeManifest => GetBoolValue("make_exe_manifest");
    public bool MakeMapFile => GetBoolValue("make_map_file");
}
