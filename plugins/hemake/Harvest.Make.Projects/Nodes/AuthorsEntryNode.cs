// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

internal class AuthorsEntryNodeTraits : NodeSetEntryBaseTraits<AuthorsNode>
{
    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "email", NodeValueDef_String.Optional() },
    };

    public override INode CreateNode(KdlNode node) => new AuthorsEntryNode(node);
}

internal class AuthorsEntryNode(KdlNode node) : NodeSetEntryBase<AuthorsEntryNodeTraits, AuthorsNode>(node)
{
    public string AuthorName => Node.Name;
    public string? AuthorEmail => TryGetValue("email", out string? value) ? value : null;
}
