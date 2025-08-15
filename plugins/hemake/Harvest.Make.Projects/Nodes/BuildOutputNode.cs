// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class BuildOutputNodeTraits : NodeBaseTraits
{
    public override string Name => "build_output";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Path.Optional(".build"),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "bin_dir", NodeValueDef_Path.Optional("${platform.name:lower}-${configuration.name:lower}/bin") },
        { "gen_dir", NodeValueDef_Path.Optional("${platform.name:lower}-${configuration.name:lower}/generated/${module.name}") },
        { "lib_dir", NodeValueDef_Path.Optional("${platform.name:lower}-${configuration.name:lower}/lib/${module.name}") },
        { "obj_dir", NodeValueDef_Path.Optional("${platform.name:lower}-${configuration.name:lower}/obj/${module.name}") },
        { "install_dir", NodeValueDef_Path.Optional("installs") },
        { "project_dir", NodeValueDef_Path.Optional("projects") },
        { "target_name", NodeValueDef_String.Optional() },
        { "target_extension", NodeValueDef_String.Optional() },
        { "make_import_lib", NodeValueDef_Bool.Optional(true) },
        { "make_exe_manifest", NodeValueDef_Bool.Optional(true) },
        { "make_map_file", NodeValueDef_Bool.Optional(false) },
    };
}

public class BuildOutputNode(KdlNode node, INode? scope) : NodeBase<BuildOutputNodeTraits>(node, scope)
{
    public string BasePath => GetPathValue(0);

    public string BinDir => GetPathValue("bin_dir");
    public string GenDir => GetPathValue("gen_dir");
    public string LibDir => GetPathValue("lib_dir");
    public string ObjDir => GetPathValue("obj_dir");
    public string InstallDir => GetPathValue("install_dir");
    public string ProjectDir => GetPathValue("project_dir");
    public string? TargetName => TryGetStringValue("target_name");
    public string? TargetExtension => TryGetStringValue("target_extension");
    public bool MakeImportLib => GetBoolValue("make_import_lib");
    public bool MakeExeManifest => GetBoolValue("make_exe_manifest");
    public bool MakeMapFile => GetBoolValue("make_map_file");

    public string GetTargetDir(EModuleKind moduleKind)
    {
        return moduleKind switch
        {
            EModuleKind.AppConsole => BinDir,
            EModuleKind.AppWindowed => BinDir,
            EModuleKind.Content => BinDir,
            EModuleKind.Custom => BinDir,
            EModuleKind.LibHeader => LibDir,
            EModuleKind.LibStatic => LibDir,
            EModuleKind.LibShared => BinDir,
            _ => LibDir,
        };
    }

    public string GetTargetExtension(EModuleKind moduleKind, bool isWindows)
    {
        return TargetExtension ?? moduleKind switch
        {
            EModuleKind.AppConsole => isWindows ? ".exe" : "",
            EModuleKind.AppWindowed => isWindows ? ".exe" : "",
            EModuleKind.Content => "",
            EModuleKind.Custom => "",
            EModuleKind.LibHeader => isWindows ? ".lib" : ".a",
            EModuleKind.LibStatic => isWindows ? ".lib" : ".a",
            EModuleKind.LibShared => isWindows ? ".dll" : ".so",
            _ => throw new Exception($"Invalid module kind '{moduleKind}' for target name."),
        };
    }
}
