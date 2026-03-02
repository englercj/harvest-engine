// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class DefinesNodeTraits : NodeSetBaseTraits<DefinesEntryNode>
{
    public override string Name => "defines";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
        PublicNode.NodeTraits.Name,
    ];

    public override ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.Include;

    public override INode CreateNode(KdlNode node) => new DefinesNode(node);
}

internal class DefinesNode(KdlNode node) : NodeSetBase<DefinesNodeTraits, DefinesEntryNode>(node)
{
}
