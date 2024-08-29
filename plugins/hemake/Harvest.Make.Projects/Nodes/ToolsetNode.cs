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

public class ToolsetNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "toolset";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EToolset>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "arch", NodeKdlEnum<EToolsetArch>.Optional },
        { "edit_and_continue", NodeKdlValue<KdlBool>.Optional },
        { "incremental_link", NodeKdlValue<KdlBool>.Optional },
        { "log", NodeKdlValue<KdlString>.Optional },
        { "multiprocess", NodeKdlValue<KdlBool>.Optional },
        { "path", NodeKdlValue<KdlString>.Optional },
        { "version", NodeKdlValue<KdlString>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public IEnumerable<string?> Tags => Node.Arguments.Select(a => (a as KdlString)?.Value);
}
