// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;
using System.Diagnostics;

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
        { "make_import_lib", NodeValueDef_Bool.Optional(true) },
        { "make_exe_manifest", NodeValueDef_Bool.Optional(true) },
        { "make_map_file", NodeValueDef_Bool.Optional(false) },
    };

    public override bool CanBeExtended => true;

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
                string targetDir = module.GetTargetDir(projectContext);
                string targetName = module.TargetName;
                string targetExtension = module.GetTargetExtension(projectContext);
                return Path.Join(targetDir, targetName + targetExtension);
            }
            case "link_target":
            {
                // Treat shared libraries as static libraries for the purpose of linking. This will let us target
                // the import library (.lib) instead of the shared library (.dll).
                EModuleKind moduleKind = projectContext.IsWindows && module.MakeImportLib && module.Kind == EModuleKind.LibShared ? EModuleKind.LibStatic : module.Kind;
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
}

public class ModuleNode(KdlNode node) : NodeBase<ModuleNodeTraits>(node)
{
    public string ModuleName => GetStringValue(0);
    public EModuleKind Kind => GetEnumValue<EModuleKind>("kind");
    public string? Group => TryGetStringValue("group");
    public EModuleLanguage Language => GetEnumValue<EModuleLanguage>("language");
    public string? ProjectFile => TryGetStringValue("project_file");
    public string? EntryPoint => TryGetStringValue("entrypoint");
    public bool IsExtension => GetBoolValue("hemake_extension");
    public string TargetName => TryGetStringValue("target_name") ?? ModuleName;
    public string? TargetExtension => TryGetStringValue("target_extension");
    public bool MakeImportLib => GetBoolValue("make_import_lib");
    public bool MakeExeManifest => GetBoolValue("make_exe_manifest");
    public bool MakeMapFile => GetBoolValue("make_map_file");

    public bool IsApp => Kind == EModuleKind.AppConsole || Kind == EModuleKind.AppWindowed;
    public bool IsBinary => IsApp || Kind == EModuleKind.LibShared;

    public string GetTargetExtension(ProjectContext projectContext)
    {
        return TryGetStringValue("target_extension") ?? Kind switch
        {
            EModuleKind.AppConsole => projectContext.IsWindows ? ".exe" : "",
            EModuleKind.AppWindowed => projectContext.IsWindows ? ".exe" : "",
            EModuleKind.Content => "",
            EModuleKind.Custom => "",
            EModuleKind.LibHeader => projectContext.IsWindows ? ".lib" : ".a",
            EModuleKind.LibStatic => projectContext.IsWindows ? ".lib" : ".a",
            EModuleKind.LibShared => projectContext.IsWindows ? ".dll" : ".so",
            _ => throw new Exception($"Invalid module kind '{Kind}' for target name."),
        };
    }

    public string GetTargetDir(ProjectContext projectContext, EModuleKind? kindOverride = null)
    {
        return GetSpecialDir(projectContext, GetSpecialDirForKind(kindOverride ?? Kind));
    }

    public string GetTargetDir(BuildOutputNode buildOutput, EModuleKind? kindOverride = null)
    {
        return GetSpecialDir(buildOutput, GetSpecialDirForKind(kindOverride ?? Kind));
    }

    public string GetBinDir(ProjectContext projectContext) => GetSpecialDir(projectContext, EModuleSpecialDir.BinDir);
    public string GetBinDir(BuildOutputNode buildOutput) => GetSpecialDir(buildOutput, EModuleSpecialDir.BinDir);

    public string GetGenDir(ProjectContext projectContext) => GetSpecialDir(projectContext, EModuleSpecialDir.GenDir);
    public string GetGenDir(BuildOutputNode buildOutput) => GetSpecialDir(buildOutput, EModuleSpecialDir.GenDir);

    public string GetLibDir(ProjectContext projectContext) => GetSpecialDir(projectContext, EModuleSpecialDir.LibDir);
    public string GetLibDir(BuildOutputNode buildOutput) => GetSpecialDir(buildOutput, EModuleSpecialDir.LibDir);

    public string GetObjDir(ProjectContext projectContext) => GetSpecialDir(projectContext, EModuleSpecialDir.ObjDir);
    public string GetObjDir(BuildOutputNode buildOutput) => GetSpecialDir(buildOutput, EModuleSpecialDir.ObjDir);

    public override void Validate(INode? scope)
    {
        base.Validate(scope);
        // TODO: Validate that dependencies are actually reasonable. For example, linking an App doesn't make sense.
    }

    private enum EModuleSpecialDir
    {
        BinDir,
        GenDir,
        LibDir,
        ObjDir,
    }

    private string GetSpecialDir(ProjectContext projectContext, EModuleSpecialDir dir)
    {
        BuildOutputNode buildOutput = projectContext.ProjectService.GetMergedNode<BuildOutputNode>(projectContext, this, false);
        return GetSpecialDir(buildOutput, dir);
    }

    private string GetSpecialDir(BuildOutputNode buildOutput, EModuleSpecialDir dir)
    {
        return dir switch
        {
            EModuleSpecialDir.BinDir => buildOutput.BinDir,
            EModuleSpecialDir.GenDir => Path.Combine(buildOutput.GenDir, TargetName),
            EModuleSpecialDir.LibDir => Path.Combine(buildOutput.LibDir, TargetName),
            EModuleSpecialDir.ObjDir => Path.Combine(buildOutput.ObjDir, TargetName),
            _ => throw new Exception($"Invalid special dir '{dir}'."),
        };
    }

    private static EModuleSpecialDir GetSpecialDirForKind(EModuleKind moduleKind)
    {
        return moduleKind switch
        {
            EModuleKind.AppConsole => EModuleSpecialDir.BinDir,
            EModuleKind.AppWindowed => EModuleSpecialDir.BinDir,
            EModuleKind.Content => EModuleSpecialDir.BinDir,
            EModuleKind.Custom => EModuleSpecialDir.BinDir,
            EModuleKind.LibHeader => EModuleSpecialDir.LibDir,
            EModuleKind.LibStatic => EModuleSpecialDir.LibDir,
            EModuleKind.LibShared => EModuleSpecialDir.BinDir,
            _ => EModuleSpecialDir.LibDir,
        };
    }
}
