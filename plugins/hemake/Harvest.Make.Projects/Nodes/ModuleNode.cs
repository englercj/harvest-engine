// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;
using System.Diagnostics;
using System.Xml.Linq;

namespace Harvest.Make.Projects.Nodes;

public enum EModuleKind
{
    [KdlName("app_console")] AppConsole,
    [KdlName("app_windowed")] AppWindowed,
    [KdlName("content")] Content,
    [KdlName("custom")] Custom,
    [KdlName("lib_header")] LibHeader,
    [KdlName("lib_static")] LibStatic,
    [KdlName("lib_shared")] LibShared,
}

public enum EModuleLanguage
{
    [KdlName("c")] C,
    [KdlName("cpp")] Cpp,
    [KdlName("csharp")] CSharp,
}

public class ModuleNodeTraits : NodeBaseTraits
{
    public override string Name => "module";

    public override IReadOnlyList<string> ValidScopes =>
    [
        PluginNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "kind", NodeValueDef_Enum<EModuleKind>.Required(EModuleKind.Custom) },
        { "group", NodeValueDef_String.Optional() },
        { "language", NodeValueDef_Enum<EModuleLanguage>.Optional(EModuleLanguage.Cpp) },
        { "project_file", NodeValueDef_String.Optional() },
        { "entrypoint", NodeValueDef_String.Optional() },
        { "hemake_extension", NodeValueDef_Bool.Optional(false) },
        { "target_name", NodeValueDef_String.Optional() },
        { "target_extension", NodeValueDef_String.Optional() },
        { "target_dir", NodeValueDef_Path.Optional() },
        { "make_import_lib", NodeValueDef_Bool.Optional(true) },
        { "make_exe_manifest", NodeValueDef_Bool.Optional(true) },
        { "make_map_file", NodeValueDef_Bool.Optional(false) },
    };

    public override bool CanBeExtended => true;

    public override INode CreateNode(KdlNode node) => new ModuleNode(node);

    public override string? TryResolveToken(ProjectContext projectContext, KdlNode contextNode, string propertyName)
    {
        Debug.Assert(contextNode.Name == Name);

        ModuleNode module = new(contextNode);

        switch (propertyName)
        {
            case "name":
            {
                return module.ModuleName;
            }
            case "build_target":
            {
                string targetDir = module.GetTargetDir(projectContext.BuildOutput);
                string targetName = module.TargetName;
                string targetExtension = module.GetTargetExtension(projectContext);
                return Path.Join(targetDir, targetName + targetExtension);
            }
            case "link_target":
            {
                // Treat shared libraries as static libraries for the purpose of linking. This will let us target
                // the import library (.lib) instead of the shared library (.dll).
                bool isWindows = projectContext.Platform.System == EPlatformSystem.Windows;
                EModuleKind moduleKind = isWindows && module.MakeImportLib && module.Kind == EModuleKind.LibShared ? EModuleKind.LibStatic : module.Kind;
                string targetDir = module.GetTargetDir(projectContext, moduleKind);
                string targetName = module.TargetName;
                string targetExtension = module.GetTargetExtension(projectContext);
                return Path.Join(targetDir, targetName + targetExtension);
            }
            case "gen_dir":
            {
                return module.GetGenDir(projectContext);
            }
        }

        return base.TryResolveToken(projectContext, contextNode, propertyName);
    }

    public override void Validate(KdlNode node)
    {
        base.Validate(node);
        // TODO: Validate that dependencies are actually reasonable. For example, linking an App doesn't make sense.
    }
}

public class ModuleNode(KdlNode node) : NodeBase<ModuleNodeTraits>(node)
{
    public string ModuleName => GetValue<string>(0);
    public EModuleKind Kind => GetEnumValue<EModuleKind>("kind");
    public string? Group => TryGetValue("group", out string? value) ? value : null;
    public EModuleLanguage Language => GetEnumValue<EModuleLanguage>("language");
    public string? ProjectFile => TryGetValue("project_file", out string? value) ? value : null;
    public string? EntryPoint => TryGetValue("entrypoint", out string? value) ? value : null;
    public bool IsExtension => GetValue<bool>("hemake_extension");
    public string TargetName => (TryGetValue("target_name", out string? value) ? value : null) ?? ModuleName;
    public string? TargetExtension => TryGetValue("target_extension", out string? value) ? value : null;
    public string? TargetDir => TryGetValue("target_dir", out string? value) ? value : null;
    public bool MakeImportLib => GetValue<bool>("make_import_lib");
    public bool MakeExeManifest => GetValue<bool>("make_exe_manifest");
    public bool MakeMapFile => GetValue<bool>("make_map_file");

    public bool IsApp => Kind == EModuleKind.AppConsole || Kind == EModuleKind.AppWindowed;
    public bool IsBinary => IsApp || Kind == EModuleKind.LibShared;

    public string GetTargetExtension(ProjectContext projectContext)
    {
        bool isWindows = projectContext.Platform.System == EPlatformSystem.Windows;

        if (TargetExtension is string extName)
        {
            return extName;
        }

        return Kind switch
        {
            EModuleKind.AppConsole => isWindows ? ".exe" : "",
            EModuleKind.AppWindowed => isWindows ? ".exe" : "",
            EModuleKind.Content => "",
            EModuleKind.Custom => "",
            EModuleKind.LibHeader => isWindows ? ".lib" : ".a",
            EModuleKind.LibStatic => isWindows ? ".lib" : ".a",
            EModuleKind.LibShared => isWindows ? ".dll" : ".so",
            _ => throw new Exception($"Invalid module kind '{Kind}' for target name."),
        };
    }

    public string GetTargetDir(BuildOutputNode buildOutput, EModuleKind? kindOverride = null)
    {
        return (kindOverride ?? Kind) switch
        {
            EModuleKind.AppConsole => GetBinDir(buildOutput),
            EModuleKind.AppWindowed => GetBinDir(buildOutput),
            EModuleKind.Content => GetBinDir(buildOutput),
            EModuleKind.Custom => GetBinDir(buildOutput),
            EModuleKind.LibHeader => GetLibDir(buildOutput),
            EModuleKind.LibStatic => GetLibDir(buildOutput),
            EModuleKind.LibShared => GetBinDir(buildOutput),
            _ => GetLibDir(buildOutput),
        };
    }

    public string GetBinDir(BuildOutputNode buildOutput) => buildOutput.BinDir;
    public string GetGenDir(BuildOutputNode buildOutput) => Path.Combine(buildOutput.GenDir, TargetName);
    public string GetLibDir(BuildOutputNode buildOutput) => Path.Combine(buildOutput.LibDir, TargetName);
    public string GetObjDir(BuildOutputNode buildOutput) => Path.Combine(buildOutput.ObjDir, TargetName);
}
