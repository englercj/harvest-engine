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

public class ModuleNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "module";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        PluginNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlString.Required(),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "kind", NodeKdlEnum<EModuleKind>.Required(EModuleKind.Custom) },
        { "group", NodeKdlString.Optional() },
        { "language", NodeKdlEnum<EModuleLanguage>.Optional(EModuleLanguage.Cpp) },
        { "project_file", NodeKdlString.Optional() },
        { "entrypoint", NodeKdlString.Optional() },
        { "hemake_extension", NodeKdlBool.Optional(false) },
    };

    public override bool CanBeExtended => true;
    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

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
