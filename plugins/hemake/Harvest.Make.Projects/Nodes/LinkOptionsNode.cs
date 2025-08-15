// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LinkOptionsNodeTraits : NodeSetBaseTraits<LinkOptionsEntryNode>
{
    public override string Name => "link_options";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "incremental_link", NodeValueDef_Bool.Optional(true) },
    };
}

public class LinkOptionsNode(KdlNode node, INode? scope) : NodeSetBase<LinkOptionsNodeTraits, LinkOptionsEntryNode>(node, scope)
{
    public bool IncrementalLink => GetBoolValue("incremental_link");
}
