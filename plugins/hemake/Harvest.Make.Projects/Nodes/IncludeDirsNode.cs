// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsNodeTraits : NodeSetBaseTraits<IncludeDirsEntryNode>
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
}

public class IncludeDirsNode(KdlNode node) : NodeSetBase<IncludeDirsNodeTraits, IncludeDirsEntryNode>(node)
{
    public bool IsExternal => GetBoolValue("external");

    protected override string GetSetEntryKey(ProjectContext context, IncludeDirsEntryNode entry)
    {
        return $"{entry.Path}|{entry.IsExternal}";
    }

    protected override void OnAddChild(ProjectContext context, IncludeDirsEntryNode entry)
    {
        KdlNode newKdlNode = new(entry.Node.Name);
        LibDirsEntryNode newEntryNode = new(newKdlNode, this);
        newEntryNode.MergeAndResolve(context, entry);

        // Store resolved value so it is properly inherited from its parent node.
        KdlValue externalValue = KdlValue.From(entry.IsExternal);
        externalValue.SourceInfo = entry.Node.SourceInfo;
        newKdlNode.Properties["external"] = externalValue;

        Children.Add(newEntryNode);
    }
}
