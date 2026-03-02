// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class SanitizeNodeTraits : NodeBaseTraits
{
    public override string Name => "sanitize";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "address", NodeValueDef_Bool.Optional(false) },
        { "fuzzer", NodeValueDef_Bool.Optional(false) },
        { "thread", NodeValueDef_Bool.Optional(false) },
        { "undefined", NodeValueDef_Bool.Optional(false) },
    };

    public override INode CreateNode(KdlNode node) => new SanitizeNode(node);
}

public class SanitizeNode(KdlNode node) : NodeBase<SanitizeNodeTraits>(node)
{
    public bool EnableAddress => GetValue<bool>("address");
    public bool EnableFuzzer => GetValue<bool>("fuzzer");
    public bool EnableThread => GetValue<bool>("thread");
    public bool EnableUndefined => GetValue<bool>("undefined");
}
