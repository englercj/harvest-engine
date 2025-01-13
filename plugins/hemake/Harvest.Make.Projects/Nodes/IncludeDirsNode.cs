// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsNode(KdlNode node, INode? scope) : NodeSetBase<IncludeDirsEntryNode>(node, scope)
{
    public const string NodeName = "include_dirs";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
        PublicNode.NodeName,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "external", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;
    public override ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.Include;

    public bool IsExternal => GetBoolValue("external");
}
