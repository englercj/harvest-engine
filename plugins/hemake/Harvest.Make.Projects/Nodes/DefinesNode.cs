// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class DefinesNode(KdlNode node, INode? scope) : NodeSetBase<DefinesEntryNode>(node, scope)
{
    public const string NodeName = "defines";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
        PublicNode.NodeName,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;
    public override ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.Include;
}
