// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class DefinesNode(KdlNode node, INode? scope) : NodeSetBase<DefinesNode, DefinesEntryNode>(node, scope)
{
    public static string NodeName => "defines";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
        PublicNode.NodeName,
    ];

    public static new ENodeDependencyInheritance NodeDependencyInheritance => ENodeDependencyInheritance.Include;
}
