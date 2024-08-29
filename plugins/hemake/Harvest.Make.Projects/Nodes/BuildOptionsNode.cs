// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class BuildOptionsNode(KdlNode node) : NodeSetBase<BuildOptionsEntryNode>(node)
{
    public const string NodeName = "build_options";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlSetAction.Optional,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "message", NodeKdlValue<KdlString>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;
}
