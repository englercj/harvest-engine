// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class LinkOptionsNodeTraits : NodeSetBaseTraits<LinkOptionsEntryNode>
{
    public override string Name => "link_options";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "incremental", NodeValueDef_Bool.Optional(true) },
    };

    public override INode CreateNode(KdlNode node) => new LinkOptionsNode(node);
}

internal class LinkOptionsNode(KdlNode node) : NodeSetBase<LinkOptionsNodeTraits, LinkOptionsEntryNode>(node)
{
    public bool IncrementalLink => GetValue<bool>("incremental");
}
