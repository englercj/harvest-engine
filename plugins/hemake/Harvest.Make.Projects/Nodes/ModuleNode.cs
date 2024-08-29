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
}

public class ModuleNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "module";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        PluginNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlValue<KdlString>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "kind", NodeKdlEnum<EModuleKind>.Required },
        { "group", NodeKdlValue<KdlString>.Optional },
        { "language", NodeKdlEnum<EModuleLanguage>.Optional },
        { "project", NodeKdlValue<KdlString>.Optional },
        { "hemake_ext", NodeKdlValue<KdlBool>.Optional },
    };

    public override bool CanBeExtended => true;
    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? ModuleName => GetStringValue(0);

    public EModuleKind Kind => GetEnumValue("kind", EModuleKind.Custom);
    public string? Group => GetStringValue("group");
    public EModuleLanguage Language => GetEnumValue("language", EModuleLanguage.Cpp);
    public string? Project => GetStringValue("project");
    public bool IsExtension => GetBoolValue("hemake_extension") ?? false;
}
