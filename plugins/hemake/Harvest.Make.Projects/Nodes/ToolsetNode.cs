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

public class ToolsetNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "toolset";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EToolset>.Required(EToolset.MSVC),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "arch", NodeKdlEnum<EToolsetArch>.Optional(EToolsetArch.Default) },
        { "edit_and_continue", NodeKdlBool.Optional(false) },
        { "fast_up_to_date_check", NodeKdlBool.Optional(true) },
        { "multiprocess", NodeKdlBool.Optional(false) },
        { "log", NodeKdlString.Optional() },
        { "path", NodeKdlPath.Optional() },
        { "version", NodeKdlString.Optional() },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EToolset Toolset => GetEnumValue<EToolset>(0);
    public EToolsetArch Arch => GetEnumValue<EToolsetArch>("arch");
    public bool EditAndContinue => GetBoolValue("edit_and_continue");
    public bool FastUpToDateCheck => GetBoolValue("fast_up_to_date_check");
    public bool MultiProcess => GetBoolValue("multiprocess");
    public string? Log => TryGetStringValue("log");
    public string? Path => TryGetPathValue("path");
    public string? Version => TryGetStringValue("version");
}
