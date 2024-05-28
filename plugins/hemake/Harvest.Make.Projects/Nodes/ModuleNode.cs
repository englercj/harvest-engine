// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class ModuleNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "module";

    public static IReadOnlyList<string> NodeScopes =
    [
        PluginNode.NodeName,
    ];

    public static IReadOnlyList<NodeArgument> NodeArguments =
    [
        NodeArgument<KdlString>.Required,
    ];

    public static IReadOnlyDictionary<string, NodeProperty> NodeProperties = new Dictionary<string, NodeProperty>()
    {
        { "kind", NodeProperty<KdlString>.Required },
        { "group", NodeProperty<KdlString>.Optional },
        { "language", NodeProperty<KdlString>.Optional },
        { "project", NodeProperty<KdlString>.Optional },
        { "hemake_ext", NodeProperty<KdlBool>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeArgument> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeProperty> Properties => NodeProperties;

}
