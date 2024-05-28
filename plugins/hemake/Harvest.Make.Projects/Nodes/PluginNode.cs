// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class PluginNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "plugin";

    public static IReadOnlyList<string> NodeScopes =>
    [
        ProjectNode.NodeName,
    ];

    public static IReadOnlyList<NodeArgument> NodeArguments =
    [
        NodeArgument<KdlString>.Required,
    ];

    public static IReadOnlyDictionary<string, NodeProperty> NodeProperties = new Dictionary<string, NodeProperty>()
    {
        { "version", NodeProperty<KdlString>.Required },
        { "license", NodeProperty<KdlString>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeArgument> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeProperty> Properties => NodeProperties;
}
