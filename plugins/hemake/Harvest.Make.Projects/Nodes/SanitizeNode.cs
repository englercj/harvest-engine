// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class SanitizeNode(KdlNode node, INode? scope) : NodeBase<SanitizeNode>(node, scope)
{
    public static string NodeName => "sanitize";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "address", NodeValueDef_Bool.Optional(false) },
        { "fuzzer", NodeValueDef_Bool.Optional(false) },
        { "thread", NodeValueDef_Bool.Optional(false) },
        { "undefined", NodeValueDef_Bool.Optional(false) },
    };

    public bool EnableAddress => GetBoolValue("address");
    public bool EnableFuzzer => GetBoolValue("fuzzer");
    public bool EnableThread => GetBoolValue("thread");
    public bool EnableUndefined => GetBoolValue("undefined");
}
