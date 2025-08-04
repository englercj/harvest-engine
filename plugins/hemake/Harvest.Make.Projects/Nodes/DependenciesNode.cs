// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class DependenciesNode(KdlNode node, INode? scope) : NodeSetBase<DependenciesNode, DependenciesEntryNode>(node, scope)
{
    public static string NodeName => "dependencies";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        PublicNode.NodeName,
    ];

    public static new ENodeDependencyInheritance NodeDependencyInheritance => ENodeDependencyInheritance.All;
}
