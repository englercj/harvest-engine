// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LibDirsNode(KdlNode node, INode? scope) : NodeSetBase<LibDirsEntryNode>(node, scope)
{
    public const string NodeName = "lib_dirs";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
        PublicNode.NodeName,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "system", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

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
        newKdlNode.Properties["system"] = KdlValue.From(entry.IsSystem);

        Children.Add(newEntryNode);
    }
}
