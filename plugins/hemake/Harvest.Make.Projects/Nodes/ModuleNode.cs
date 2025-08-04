// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

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

public class ModuleNode(KdlNode node, INode? scope) : NodeBase<ModuleNode>(node, scope)
{
    public static string NodeName => "module";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        PluginNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "kind", NodeValueDef_Enum<EModuleKind>.Required(EModuleKind.Custom) },
        { "group", NodeValueDef_String.Optional() },
        { "language", NodeValueDef_Enum<EModuleLanguage>.Optional(EModuleLanguage.Cpp) },
        { "project_file", NodeValueDef_String.Optional() },
        { "entrypoint", NodeValueDef_String.Optional() },
        { "hemake_extension", NodeValueDef_Bool.Optional(false) },
    };

    public static new bool NodeCanBeExtended => true;

    public string ModuleName => GetStringValue(0);
    public EModuleKind Kind => GetEnumValue<EModuleKind>("kind");
    public string? Group => TryGetStringValue("group");
    public EModuleLanguage Language => GetEnumValue<EModuleLanguage>("language");
    public string? ProjectFile => TryGetStringValue("project_file");
    public string? EntryPoint => TryGetStringValue("entrypoint");
    public bool IsExtension => GetBoolValue("hemake_extension");

    public bool IsApp => Kind == EModuleKind.AppConsole || Kind == EModuleKind.AppWindowed;
    public bool IsBinary => IsApp || Kind == EModuleKind.LibShared;

    // TODO: Validate that dependencies are actually reasonable. For example, linking an App doesn't make sense.
}
