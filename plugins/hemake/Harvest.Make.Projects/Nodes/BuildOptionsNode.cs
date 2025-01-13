// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EBuildClrMode
{
    [KdlName("on")] On,
    [KdlName("off")] Off,
    [KdlName("netcore")] NetCore,
}

public enum EBuildMfcMode
{
    [KdlName("off")] Off,
    [KdlName("static")] Static,
    [KdlName("dynamic")] Dynamic,
}

public enum EBuildAtlMode
{
    [KdlName("off")] Off,
    [KdlName("static")] Static,
    [KdlName("dynamic")] Dynamic,
}

public class BuildOptionsNode(KdlNode node, INode? scope) : NodeSetBase<BuildOptionsEntryNode>(node, scope)
{
    public const string NodeName = "build_options";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "clr", NodeKdlEnum<EBuildClrMode>.Optional(EBuildClrMode.Off) },
        { "mfc", NodeKdlEnum<EBuildMfcMode>.Optional(EBuildMfcMode.Off) },
        { "atl", NodeKdlEnum<EBuildAtlMode>.Optional(EBuildAtlMode.Off) },
        { "run_code_analysis", NodeKdlBool.Optional(false) },
        { "run_clang_tidy", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;
    public override ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.Include;

    public EBuildClrMode ClrMode => GetEnumValue<EBuildClrMode>("clr");
    public EBuildMfcMode MfcMode => GetEnumValue<EBuildMfcMode>("mfc");
    public EBuildAtlMode AtlMode => GetEnumValue<EBuildAtlMode>("atl");
    public bool RunCodeAnalysis => GetBoolValue("run_code_analysis");
    public bool RunClangTidy => GetBoolValue("run_clang_tidy");
}
