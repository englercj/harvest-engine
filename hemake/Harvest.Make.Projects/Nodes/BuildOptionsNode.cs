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

public enum EDpiAwareMode
{
    [KdlName("none")] None,
    [KdlName("high")] HighDpiAware,
    [KdlName("high_permonitor")] PerMonitorHighDpiAware,
}

public class BuildOptionsNodeTraits : NodeSetBaseTraits<BuildOptionsEntryNode>
{
    public override string Name => "build_options";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
        PublicNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "clr", NodeValueDef_Enum<EBuildClrMode>.Optional(EBuildClrMode.Off) },
        { "mfc", NodeValueDef_Enum<EBuildMfcMode>.Optional(EBuildMfcMode.Off) },
        { "atl", NodeValueDef_Enum<EBuildAtlMode>.Optional(EBuildAtlMode.Off) },
        { "dpiawareness", NodeValueDef_Enum<EDpiAwareMode>.Optional(EDpiAwareMode.None) },
        { "pch_include", NodeValueDef_String.Optional() },
        { "pch_source", NodeValueDef_Path.Optional() },
        { "rtti", NodeValueDef_Bool.Optional(false) },
        { "run_code_analysis", NodeValueDef_Bool.Optional(false) },
        { "run_clang_tidy", NodeValueDef_Bool.Optional(false) },
        { "omit_default_lib", NodeValueDef_Bool.Optional(false) },
        { "omit_frame_pointers", NodeValueDef_Bool.Optional(false) },
        { "openmp", NodeValueDef_Bool.Optional(false) },
    };

    public override ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.Include;

    protected override void ValidateProperties(KdlNode node)
    {
        base.ValidateProperties(node);

        bool hasPchInclude = node.HasValue("pch_include");
        bool hasPchSource = node.HasValue("pch_source");
        if (hasPchInclude != hasPchSource)
        {
            throw new NodeParseException(node, "If you specify 'pch_include' or 'pch_source', you must specify both.");
        }
    }

    public override INode CreateNode(KdlNode node) => new BuildOptionsNode(node);
}

public class BuildOptionsNode(KdlNode node) : NodeSetBase<BuildOptionsNodeTraits, BuildOptionsEntryNode>(node)
{
    public EBuildClrMode ClrMode => GetEnumValue<EBuildClrMode>("clr");
    public EBuildMfcMode MfcMode => GetEnumValue<EBuildMfcMode>("mfc");
    public EBuildAtlMode AtlMode => GetEnumValue<EBuildAtlMode>("atl");
    public EDpiAwareMode DpiAwarenessMode => GetEnumValue<EDpiAwareMode>("dpiawareness");
    public string? PchInclude => TryGetValue("pch_include", out string? value) ? value : null;
    public string? PchSource => TryGetValue("pch_source", out string? value) ? value : null;
    public bool RuntimeTypeInfo => GetValue<bool>("rtti");
    public bool RunCodeAnalysis => GetValue<bool>("run_code_analysis");
    public bool RunClangTidy => GetValue<bool>("run_clang_tidy");
    public bool OmitDefaultLib => GetValue<bool>("omit_default_lib");
    public bool OmitFramePointers => GetValue<bool>("omit_frame_pointers");
    public bool OpenMP => GetValue<bool>("openmp");
}
