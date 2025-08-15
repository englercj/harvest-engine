// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LibDirsNodeTraits : NodeSetBaseTraits<LibDirsEntryNode>
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
}

public class LibDirsNode(KdlNode node, INode? scope) : NodeSetBase<LibDirsNodeTraits, LibDirsEntryNode>(node, scope)
{
    public bool IsSystem => GetBoolValue("system");

    protected override string GetSetEntryKey(ProjectContext context, LibDirsEntryNode entry)
    {
        return $"{entry.Path}|{entry.IsSystem}";
    }

    protected override void OnAddChild(ProjectContext context, LibDirsEntryNode entry)
    {
        KdlNode newKdlNode = new(entry.Node.Name);
        LibDirsEntryNode newEntryNode = new(newKdlNode, this);
        newEntryNode.MergeAndResolve(context, entry);

        // Store resolved value so it is properly inherited from its parent node.
        KdlValue systemValue = KdlValue.From(entry.IsSystem);
        systemValue.SourceInfo = entry.Node.SourceInfo;
        newKdlNode.Properties["system"] = systemValue;

        Children.Add(newEntryNode);
    }
}
