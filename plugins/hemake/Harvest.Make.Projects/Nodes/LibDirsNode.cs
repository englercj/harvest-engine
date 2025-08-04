// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LibDirsNode(KdlNode node, INode? scope) : NodeSetBase<LibDirsNode, LibDirsEntryNode>(node, scope)
{
    public static string NodeName => "lib_dirs";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
        PublicNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "system", NodeValueDef_Bool.Optional(false) },
    };

    public static new ENodeDependencyInheritance NodeDependencyInheritance => ENodeDependencyInheritance.Link;

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
