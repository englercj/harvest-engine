// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class AuthorsEntryNode(KdlNode node, INode? scope) : NodeBase<AuthorsEntryNode>(node, scope)
{
    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        AuthorsNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "email", NodeValueDef_String.Optional() },
    };

    public string AuthorName => Node.Name;
    public string? AuthorEmail => TryGetStringValue("email");
}
