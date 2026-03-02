// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class LibDirsNodeTraits : NodeSetBaseTraits<LibDirsEntryNode>
{
    public override string Name => "lib_dirs";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
        PublicNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "system", NodeValueDef_Bool.Optional(false) },
    };

    public override ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.Link;

    public override INode CreateNode(KdlNode node) => new LibDirsNode(node);
}

internal class LibDirsNode(KdlNode node) : NodeSetBase<LibDirsNodeTraits, LibDirsEntryNode>(node)
{
    public bool IsSystem => GetValue<bool>("system");

    protected override string GetSetEntryKey(ProjectContext context, KdlNode entry)
    {
        LibDirsEntryNode libDirsEntry = new(entry);
        return $"{libDirsEntry.Path}|{libDirsEntry.IsSystem}";
    }
}
