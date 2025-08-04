// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class IncludeDirsNode(KdlNode node, INode? scope) : NodeSetBase<IncludeDirsNode, IncludeDirsEntryNode>(node, scope)
{
    public static string NodeName => "include_dirs";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
        PublicNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "external", NodeValueDef_Bool.Optional(false) },
    };

    public static new ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.Include;

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
