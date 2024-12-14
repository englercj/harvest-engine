// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
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
    [KdlName("fsharp")] FSharp,
}

public enum EModuleClrMode
{
    [KdlName("on")] On,
    [KdlName("off")] Off,
    [KdlName("netcore")] NetCore,
}

public enum EModuleMfcMode
{
    [KdlName("off")] Off,
    [KdlName("static")] Static,
    [KdlName("dynamic")] Dynamic,
}

public enum EModuleAtlMode
{
    [KdlName("off")] Off,
    [KdlName("static")] Static,
    [KdlName("dynamic")] Dynamic,
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
        { "clr", NodeKdlEnum<EModuleClrMode>.Optional(EModuleClrMode.Off) },
        { "mfc", NodeKdlEnum<EModuleMfcMode>.Optional(EModuleMfcMode.Off) },
        { "atl", NodeKdlEnum<EModuleAtlMode>.Optional(EModuleAtlMode.Off) },
        { "group", NodeKdlString.Optional() },
        { "language", NodeKdlEnum<EModuleLanguage>.Optional(EModuleLanguage.Cpp) },
        { "project", NodeKdlString.Optional() },
        { "hemake_extension", NodeKdlBool.Optional(false) },
    };

    public override bool CanBeExtended => true;
    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string ModuleName => GetStringValue(0);
    public EModuleKind Kind => GetEnumValue<EModuleKind>("kind");
    public EModuleClrMode ClrMode => GetEnumValue<EModuleClrMode>("clr");
    public EModuleMfcMode MfcMode => GetEnumValue<EModuleMfcMode>("mfc");
    public EModuleAtlMode AtlMode => GetEnumValue<EModuleAtlMode>("atl");
    public string? Group => TryGetStringValue("group");
    public EModuleLanguage Language => GetEnumValue<EModuleLanguage>("language");
    public string? Project => TryGetStringValue("project");
    public bool IsExtension => GetBoolValue("hemake_extension");
}
