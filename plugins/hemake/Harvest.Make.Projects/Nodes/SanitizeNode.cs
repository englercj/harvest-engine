// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class SanitizeNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "sanitize";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "address", NodeKdlBool.Optional(false) },
        { "fuzzer", NodeKdlBool.Optional(false) },
        { "thread", NodeKdlBool.Optional(false) },
        { "undefined", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public bool EnableAddress => GetBoolValue("address");
    public bool EnableFuzzer => GetBoolValue("fuzzer");
    public bool EnableThread => GetBoolValue("thread");
    public bool EnableUndefined => GetBoolValue("undefined");
}
