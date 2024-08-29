// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public class SanitizeNode(KdlNode node) : NodeBase(node)
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

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "address", NodeKdlValue<KdlBool>.Optional },
        { "fuzzer", NodeKdlValue<KdlBool>.Optional },
        { "thread", NodeKdlValue<KdlBool>.Optional },
        { "undefined", NodeKdlValue<KdlBool>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public bool EnableAddress => GetBoolValue("address") ?? false;
    public bool EnableFuzzer => GetBoolValue("fuzzer") ?? false;
    public bool EnableThread => GetBoolValue("thread") ?? false;
    public bool EnableUndefined => GetBoolValue("undefined") ?? false;
}
