// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class AuthorsEntryNodeTraits : NodeSetEntryBaseTraits<AuthorsNode>
{
    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "email", NodeValueDef_String.Optional() },
    };
}

public class AuthorsEntryNode(KdlNode node) : NodeSetEntryBase<AuthorsEntryNodeTraits, AuthorsNode>(node)
{
    public string AuthorName => Node.Name;
    public string? AuthorEmail => TryGetStringValue("email");
}
