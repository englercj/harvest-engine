// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class ProjectNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "project";

    public IReadOnlyList<string> NodeScopes =>
    [
    ];

    public IReadOnlyList<NodeArgument> NodeArguments =>
    [
        NodeArgument<KdlString>.Required,
    ];

    public IReadOnlyDictionary<string, NodeProperty> NodeProperties => new Dictionary<string, NodeProperty>()
    {
        { "start", NodeProperty<KdlString>.Optional },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeArgument> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeProperty> Properties => NodeProperties;
}
