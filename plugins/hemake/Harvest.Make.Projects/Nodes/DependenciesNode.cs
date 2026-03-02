// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class DependenciesNodeTraits : NodeSetBaseTraits<DependenciesEntryNode>
{
    public override string Name => "dependencies";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        PublicNode.NodeTraits.Name,
    ];

    public override ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.All;

    public override INode CreateNode(KdlNode node) => new DependenciesNode(node);
}

public class DependenciesNode(KdlNode node) : NodeSetBase<DependenciesNodeTraits, DependenciesEntryNode>(node)
{
}
