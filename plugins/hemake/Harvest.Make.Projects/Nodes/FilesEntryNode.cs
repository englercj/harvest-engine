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

internal class ExtensionInfo
{
    public ExtensionInfo(EFileAction action)
    {
        Action = action;
        BuildRule = EFileBuildRule.Default;
    }

    public ExtensionInfo(EFileBuildRule rule)
    {
        Action = EFileAction.Build;
        BuildRule = rule;
    }

    public EFileAction Action { get; }
    public EFileBuildRule BuildRule { get; }
}

public class FilesEntryNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    private static readonly Dictionary<string, ExtensionInfo> s_fileExtensionInfos = new()
    {
        { ".appxmanifest", new ExtensionInfo(EFileAction.AppxManifest) },
        { ".asm", new ExtensionInfo(EFileBuildRule.Asm) },
        { ".s", new ExtensionInfo(EFileBuildRule.Asm) },
        { ".S", new ExtensionInfo(EFileBuildRule.Asm) },
        { ".c", new ExtensionInfo(EFileBuildRule.C) },
        { ".cc", new ExtensionInfo(EFileBuildRule.Cpp) },
        { ".cpp", new ExtensionInfo(EFileBuildRule.Cpp) },
        { ".cppm", new ExtensionInfo(EFileBuildRule.Cpp) },
        { ".cxx", new ExtensionInfo(EFileBuildRule.Cpp) },
        { ".c++", new ExtensionInfo(EFileBuildRule.Cpp) },
        { ".ixx", new ExtensionInfo(EFileBuildRule.Cpp) },
        { ".cs", new ExtensionInfo(EFileBuildRule.CSharp) },
        { ".m", new ExtensionInfo(EFileBuildRule.ObjC) },
        { ".mm", new ExtensionInfo(EFileBuildRule.ObjCpp) },
        { ".idl", new ExtensionInfo(EFileBuildRule.Midl) },
        { ".a", new ExtensionInfo(EFileAction.Framework) },
        { ".dylib", new ExtensionInfo(EFileAction.Framework) },
        { ".framework", new ExtensionInfo(EFileAction.Framework) },
        { ".gif", new ExtensionInfo(EFileAction.Image) },
        { ".jpg", new ExtensionInfo(EFileAction.Image) },
        { ".jpeg", new ExtensionInfo(EFileAction.Image) },
        { ".png", new ExtensionInfo(EFileAction.Image) },
        { ".bmp", new ExtensionInfo(EFileAction.Image) },
        { ".dib", new ExtensionInfo(EFileAction.Image) },
        { ".tif", new ExtensionInfo(EFileAction.Image) },
        { ".wmf", new ExtensionInfo(EFileAction.Image) },
        { ".ras", new ExtensionInfo(EFileAction.Image) },
        { ".eps", new ExtensionInfo(EFileAction.Image) },
        { ".pcx", new ExtensionInfo(EFileAction.Image) },
        { ".pcd", new ExtensionInfo(EFileAction.Image) },
        { ".tga", new ExtensionInfo(EFileAction.Image) },
        { ".dds", new ExtensionInfo(EFileAction.Image) },
        { ".h", new ExtensionInfo(EFileAction.Include) },
        { ".hh", new ExtensionInfo(EFileAction.Include) },
        { ".hpp", new ExtensionInfo(EFileAction.Include) },
        { ".hxx", new ExtensionInfo(EFileAction.Include) },
        { ".inl", new ExtensionInfo(EFileAction.Include) },
        { ".manifest", new ExtensionInfo(EFileAction.Manifest) },
        { ".natvis", new ExtensionInfo(EFileAction.Natvis) },
        { ".rc", new ExtensionInfo(EFileAction.Resource) },
        { ".metal", new ExtensionInfo(EFileAction.Resource) },
        { ".strings", new ExtensionInfo(EFileAction.Resource) },
        { ".nib", new ExtensionInfo(EFileAction.Resource) },
        { ".xib", new ExtensionInfo(EFileAction.Resource) },
        { ".storyboard", new ExtensionInfo(EFileAction.Resource) },
        { ".icns", new ExtensionInfo(EFileAction.Resource) },
        { ".xcprivacy", new ExtensionInfo(EFileAction.Resource) },
    };

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        FilesNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "action", NodeKdlEnum<EFileAction>.Optional(EFileAction.Default) },
        { "build_rule", NodeKdlString.Optional("default") }, // string to support custom build rule names
        { "build_exclude", NodeKdlBool.Optional(false) },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string FileGlob => Node.Name;
    public IEnumerable<string> FilePaths => ExpandPath(ResolvePath(FileGlob));
    public EFileAction FileAction => GetEnumValue<EFileAction>("action");
    public EFileBuildRule FileBuildRule => TryGetEnumValue<EFileBuildRule>("build_rule") ?? EFileBuildRule.Custom;
    public string BuildRuleName => GetStringValue("build_rule");
    public bool IsExcludedFromBuild => GetBoolValue("build_exclude");

    private string? _resolvedPath = null;
    public string ResolvedFilePath => _resolvedPath ?? throw new Exception("File path has not been resolved yet.");

    public EFileAction ResolvedFileAction => FileAction switch
    {
        EFileAction.Default => GetDefaultFileAction(ResolvedFilePath),
        _ => FileAction,
    };

    public EFileBuildRule ResolvedFileBuildRule => FileBuildRule switch
    {
        EFileBuildRule.Default => GetDefaultFileBuildRule(ResolvedFilePath),
        _ => FileBuildRule,
    };

    public static EFileAction GetDefaultFileAction(string path)
    {
        if (s_fileExtensionInfos.TryGetValue(Path.GetExtension(path), out ExtensionInfo? info))
        {
            return info.Action;
        }

        return EFileAction.None;
    }

    public static EFileBuildRule GetDefaultFileBuildRule(string path)
    {
        if (s_fileExtensionInfos.TryGetValue(Path.GetExtension(path), out ExtensionInfo? info))
        {
            return info.BuildRule;
        }

        return EFileBuildRule.Custom;
    }

    public override NodeValidationResult Validate(INode? scope)
    {
        NodeValidationResult vr = base.Validate(scope);
        if (!vr.IsValid)
        {
            return vr;
        }

        if (BuildRuleName is not null && FileAction != EFileAction.Build)
        {
            return NodeValidationResult.Error("Rule can only be specified for files with action 'build'");
        }

        return NodeValidationResult.Valid;
    }

    public override void MergeAndResolve(ProjectContext context, INode node)
    {
        base.MergeAndResolve(context, node);

        // After merging `FileGlob` should only be a single file path from the original
        // expanded glob. We store it off so we know this is the final file path.
        _resolvedPath = FileGlob;

        if (FileAction == EFileAction.Default)
        {
            EFileAction fileAction = GetDefaultFileAction(_resolvedPath);
            string fileActionName = KdlEnumUtils.GetName(fileAction);
            Node.Properties["action"] = KdlValue.From(fileActionName);
        }

        if (FileBuildRule == EFileBuildRule.Default)
        {
            EFileBuildRule fileRule = GetDefaultFileBuildRule(_resolvedPath);
            string fileRuleName = KdlEnumUtils.GetName(fileRule);
            Node.Properties["build_rule"] = KdlValue.From(fileRuleName);
        }
    }
}
