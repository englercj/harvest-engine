// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class ImportNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "import";

    public static IReadOnlyList<string> NodeScopes =>
    [
        // TODO: install node
        ModuleNode.NodeName,
        PluginNode.NodeName,
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

    public string? ImportPath => (Node.Arguments[0] as KdlString)?.Value;
}
