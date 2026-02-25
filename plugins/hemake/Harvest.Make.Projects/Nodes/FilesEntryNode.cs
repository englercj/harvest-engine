// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EFileAction
{
    [KdlName("default")] Default,
    [KdlName("none")] None,
    [KdlName("appxmanifest")] AppxManifest,
    [KdlName("build")] Build,
    [KdlName("copy")] Copy,
    [KdlName("framework")] Framework,
    [KdlName("image")] Image,
    [KdlName("include")] Include,
    [KdlName("manifest")] Manifest,
    [KdlName("natvis")] Natvis,
    [KdlName("resource")] Resource,
}

public enum EFileBuildRule
{
    [KdlName("default")] Default,
    [KdlName("asm")] Asm,
    [KdlName("c")] C,
    [KdlName("cpp")] Cpp,
    [KdlName("csharp")] CSharp,
    [KdlName("objc")] ObjC,
    [KdlName("objcpp")] ObjCpp,
    [KdlName("midl")] Midl,
    [KdlName("custom")] Custom,
}

public class FileEntryExtensionInfo
{
    public FileEntryExtensionInfo(EFileAction action)
    {
        Action = action;
        BuildRule = EFileBuildRule.Default;
    }

    public FileEntryExtensionInfo(EFileBuildRule rule)
    {
        Action = EFileAction.Build;
        BuildRule = rule;
    }

    public EFileAction Action { get; }
    public EFileBuildRule BuildRule { get; }
}

public class FileEntryNodeTraits : NodeSetEntryBaseTraits<FilesNode>
{
    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "action", NodeValueDef_Enum<EFileAction>.Optional(EFileAction.Default) },
        { "build_rule", NodeValueDef_String.Optional("default") }, // string to support custom build rule names
        { "build_exclude", NodeValueDef_Bool.Optional(false) },
    };

    public override INode CreateNode(KdlNode node) => new FilesEntryNode(node);
}

public class FilesEntryNode(KdlNode node) : NodeSetEntryBase<FileEntryNodeTraits, FilesNode>(node)
{
    private static readonly Dictionary<string, FileEntryExtensionInfo> s_fileExtensionInfos = new()
    {
        { ".appxmanifest", new FileEntryExtensionInfo(EFileAction.AppxManifest) },
        { ".asm", new FileEntryExtensionInfo(EFileBuildRule.Asm) },
        { ".s", new FileEntryExtensionInfo(EFileBuildRule.Asm) },
        { ".S", new FileEntryExtensionInfo(EFileBuildRule.Asm) },
        { ".c", new FileEntryExtensionInfo(EFileBuildRule.C) },
        { ".cc", new FileEntryExtensionInfo(EFileBuildRule.Cpp) },
        { ".cpp", new FileEntryExtensionInfo(EFileBuildRule.Cpp) },
        { ".cppm", new FileEntryExtensionInfo(EFileBuildRule.Cpp) },
        { ".cxx", new FileEntryExtensionInfo(EFileBuildRule.Cpp) },
        { ".c++", new FileEntryExtensionInfo(EFileBuildRule.Cpp) },
        { ".ixx", new FileEntryExtensionInfo(EFileBuildRule.Cpp) },
        { ".cs", new FileEntryExtensionInfo(EFileBuildRule.CSharp) },
        { ".m", new FileEntryExtensionInfo(EFileBuildRule.ObjC) },
        { ".mm", new FileEntryExtensionInfo(EFileBuildRule.ObjCpp) },
        { ".idl", new FileEntryExtensionInfo(EFileBuildRule.Midl) },
        { ".a", new FileEntryExtensionInfo(EFileAction.Framework) },
        { ".dylib", new FileEntryExtensionInfo(EFileAction.Framework) },
        { ".framework", new FileEntryExtensionInfo(EFileAction.Framework) },
        { ".gif", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".jpg", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".jpeg", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".png", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".bmp", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".dib", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".tif", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".wmf", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".ras", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".eps", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".pcx", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".pcd", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".tga", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".dds", new FileEntryExtensionInfo(EFileAction.Image) },
        { ".h", new FileEntryExtensionInfo(EFileAction.Include) },
        { ".hh", new FileEntryExtensionInfo(EFileAction.Include) },
        { ".hpp", new FileEntryExtensionInfo(EFileAction.Include) },
        { ".hxx", new FileEntryExtensionInfo(EFileAction.Include) },
        { ".inl", new FileEntryExtensionInfo(EFileAction.Include) },
        { ".manifest", new FileEntryExtensionInfo(EFileAction.Manifest) },
        { ".natvis", new FileEntryExtensionInfo(EFileAction.Natvis) },
        { ".rc", new FileEntryExtensionInfo(EFileAction.Resource) },
        { ".metal", new FileEntryExtensionInfo(EFileAction.Resource) },
        { ".strings", new FileEntryExtensionInfo(EFileAction.Resource) },
        { ".nib", new FileEntryExtensionInfo(EFileAction.Resource) },
        { ".xib", new FileEntryExtensionInfo(EFileAction.Resource) },
        { ".storyboard", new FileEntryExtensionInfo(EFileAction.Resource) },
        { ".icns", new FileEntryExtensionInfo(EFileAction.Resource) },
        { ".xcprivacy", new FileEntryExtensionInfo(EFileAction.Resource) },
    };

    public static void RegisterFileExtension(string extension, FileEntryExtensionInfo info)
    {
        s_fileExtensionInfos[extension] = info;
    }

    public string FilePath => Node.Name;
    public EFileAction FileAction => GetFileAction();
    public EFileBuildRule FileBuildRule => GetFileBuildRule();
    public string BuildRuleName => GetValue<string>("build_rule");
    public bool IsExcludedFromBuild => GetValue<bool>("build_exclude");

    public static EFileAction GetDefaultFileAction(string path)
    {
        if (s_fileExtensionInfos.TryGetValue(Path.GetExtension(path), out FileEntryExtensionInfo? info))
        {
            return info.Action;
        }

        return EFileAction.None;
    }

    public static EFileBuildRule GetDefaultFileBuildRule(string path)
    {
        if (s_fileExtensionInfos.TryGetValue(Path.GetExtension(path), out FileEntryExtensionInfo? info))
        {
            return info.BuildRule;
        }

        return EFileBuildRule.Custom;
    }

    private EFileAction GetFileAction()
    {
        EFileAction action = GetEnumValue<EFileAction>("action");
        return action == EFileAction.Default ? GetDefaultFileAction(FilePath) : action;
    }

    private EFileBuildRule GetFileBuildRule()
    {
        string buildRuleName = GetValue<string>("build_rule");

        if (!KdlEnumUtils.TryParse(buildRuleName, out EFileBuildRule rule))
        {
            return EFileBuildRule.Custom;
        }

        return rule == EFileBuildRule.Default ? GetDefaultFileBuildRule(FilePath) : rule;
    }
}
