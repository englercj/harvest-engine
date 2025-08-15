// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EToolset
{
    [KdlName("clang")] Clang,
    [KdlName("gcc")] GCC,
    [KdlName("msvc")] MSVC,
}

public enum EToolsetArch
{
    [KdlName("default")] Default,
    [KdlName("x86")] X86,
    [KdlName("x86_64")] X86_64,
}

public class ToolsetNodeTraits : NodeBaseTraits
{
    public override string Name => "toolset";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EToolset>.Required(EToolset.MSVC),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "arch", NodeValueDef_Enum<EToolsetArch>.Optional(EToolsetArch.Default) },
        { "edit_and_continue", NodeValueDef_Bool.Optional(false) },
        { "fast_up_to_date_check", NodeValueDef_Bool.Optional(true) },
        { "multiprocess", NodeValueDef_Bool.Optional(false) },
        { "log", NodeValueDef_String.Optional() },
        { "path", NodeValueDef_Path.Optional() },
        { "version", NodeValueDef_String.Optional() },
    };
}

public class ToolsetNode(KdlNode node, INode? scope) : NodeBase<ToolsetNodeTraits>(node, scope)
{
    public EToolset Toolset => GetEnumValue<EToolset>(0);
    public EToolsetArch Arch => GetEnumValue<EToolsetArch>("arch");
    public bool EditAndContinue => GetBoolValue("edit_and_continue");
    public bool FastUpToDateCheck => GetBoolValue("fast_up_to_date_check");
    public bool MultiProcess => GetBoolValue("multiprocess");
    public string? Log => TryGetStringValue("log");
    public string? Path => TryGetPathValue("path");
    public string? Version => TryGetStringValue("version");
}
