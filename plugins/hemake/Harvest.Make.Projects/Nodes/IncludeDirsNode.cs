// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsNode(KdlNode node) : NodeSetBase<IncludeDirsEntryNode>(node)
{
    public const string NodeName = "include_dirs";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
        PrivateNode.NodeName,
        PublicNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlSetAction.Optional,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "external", NodeKdlValue<KdlBool>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public bool IsExternal => GetBoolValue("external") ?? false;
}
