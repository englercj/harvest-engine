// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class IncludeDirsNodeTraits : NodeSetBaseTraits<IncludeDirsEntryNode>
{
    public override string Name => "include_dirs";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
        PublicNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "external", NodeValueDef_Bool.Optional(false) },
    };

    public override ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.Include;

    public override INode CreateNode(KdlNode node) => new IncludeDirsNode(node);
}

internal class IncludeDirsNode(KdlNode node) : NodeSetBase<IncludeDirsNodeTraits, IncludeDirsEntryNode>(node)
{
    public bool IsExternal => GetValue<bool>("external");

    protected override string GetSetEntryKey(ProjectContext context, KdlNode entry)
    {
        IncludeDirsEntryNode includeDirsEntry = new(entry);
        return $"{includeDirsEntry.Path}|{includeDirsEntry.IsExternal}";
    }
}
