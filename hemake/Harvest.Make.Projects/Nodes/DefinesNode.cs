// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class DefinesNodeTraits : NodeSetBaseTraits<DefinesEntryNode>
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

public class DefinesNode(KdlNode node) : NodeSetBase<DefinesNodeTraits, DefinesEntryNode>(node)
{
}
