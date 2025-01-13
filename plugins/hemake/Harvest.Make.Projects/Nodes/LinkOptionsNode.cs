// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LinkOptionsNode(KdlNode node, INode? scope) : NodeSetBase<LinkOptionsEntryNode>(node, scope)
{
    public const string NodeName = "link_options";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "incremental_link", NodeKdlBool.Optional(true) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public bool IncrementalLink => GetBoolValue("incremental_link");
}
