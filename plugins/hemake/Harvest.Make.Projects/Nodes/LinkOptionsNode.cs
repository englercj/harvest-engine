// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LinkOptionsNode(KdlNode node, INode? scope) : NodeSetBase<LinkOptionsNode, LinkOptionsEntryNode>(node, scope)
{
    public static string NodeName => "link_options";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "incremental_link", NodeValueDef_Bool.Optional(true) },
    };

    public bool IncrementalLink => GetBoolValue("incremental_link");
}
