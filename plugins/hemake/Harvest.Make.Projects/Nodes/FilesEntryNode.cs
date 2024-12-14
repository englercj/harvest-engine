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
    [KdlName("natvis")] Natvis,
    [KdlName("resource")] Resource,
}

public enum EFileRule
{
    [KdlName("default")] Default,
    [KdlName("asm")] Asm,
    [KdlName("c")] C,
    [KdlName("cpp")] Cpp,
    [KdlName("csharp")] CSharp,
    [KdlName("fx")] Fx,
    [KdlName("objc")] ObjC,
    [KdlName("objcpp")] ObjCpp,
    [KdlName("midl")] Midl,
    [KdlName("swift")] Swift,
    [KdlName("custom")] Custom,
}

internal class ExtensionInfo
{
    public ExtensionInfo(EFileAction action)
    {
        Action = action;
        Rule = EFileRule.Custom;
    }

    public ExtensionInfo(EFileAction action, EFileRule rule)
    {
        Action = action;
        Rule = rule;
    }

    public EFileAction Action { get; }
    public EFileRule Rule { get; }
}

public class FilesEntryNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    private static readonly Dictionary<string, ExtensionInfo> s_fileExtensionInfos = new()
    {
        { ".appxmanifest", new ExtensionInfo(EFileAction.AppxManifest) },
        { ".asm", new ExtensionInfo(EFileAction.Build, EFileRule.Asm) },
        { ".s", new ExtensionInfo(EFileAction.Build, EFileRule.Asm) },
        { ".S", new ExtensionInfo(EFileAction.Build, EFileRule.Asm) },
        { ".c", new ExtensionInfo(EFileAction.Build, EFileRule.C) },
        { ".cc", new ExtensionInfo(EFileAction.Build, EFileRule.Cpp) },
        { ".cpp", new ExtensionInfo(EFileAction.Build, EFileRule.Cpp) },
        { ".cppm", new ExtensionInfo(EFileAction.Build, EFileRule.Cpp) },
        { ".cxx", new ExtensionInfo(EFileAction.Build, EFileRule.Cpp) },
        { ".c++", new ExtensionInfo(EFileAction.Build, EFileRule.Cpp) },
        { ".ixx", new ExtensionInfo(EFileAction.Build, EFileRule.Cpp) },
        { ".cs", new ExtensionInfo(EFileAction.Build, EFileRule.CSharp) },
        { ".hlsl", new ExtensionInfo(EFileAction.Build, EFileRule.Fx) },
        { ".m", new ExtensionInfo(EFileAction.Build, EFileRule.ObjC) },
        { ".mm", new ExtensionInfo(EFileAction.Build, EFileRule.ObjCpp) },
        { ".idl", new ExtensionInfo(EFileAction.Build, EFileRule.Midl) },
        { ".swift", new ExtensionInfo(EFileAction.Build, EFileRule.Swift) },
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
        { "rule", NodeKdlString.Optional("default") },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string FileGlob => Node.Name;
    public IEnumerable<string> FilePaths => ExpandPath(FileGlob);
    public EFileAction FileAction => GetEnumValue<EFileAction>("action");
    public EFileRule FileRule => GetEnumValue<EFileRule>("rule");
    public string RuleName => GetStringValue("rule");

    public static EFileAction GetDefaultFileAction(string path)
    {
        if (s_fileExtensionInfos.TryGetValue(Path.GetExtension(path), out ExtensionInfo? info))
        {
            return info.Action;
        }

        return EFileAction.None;
    }

    public static EFileRule GetDefaultFileRule(string path)
    {
        if (s_fileExtensionInfos.TryGetValue(Path.GetExtension(path), out ExtensionInfo? info))
        {
            return info.Rule;
        }

        return EFileRule.Custom;
    }

    public override NodeValidationResult Validate(INode? scope)
    {
        NodeValidationResult vr = base.Validate(scope);
        if (!vr.IsValid)
        {
            return vr;
        }

        if (RuleName is not null && FileAction != EFileAction.Build)
        {
            return NodeValidationResult.Error("Rule can only be specified for files with action 'build'");
        }

        return NodeValidationResult.Valid;
    }
}
