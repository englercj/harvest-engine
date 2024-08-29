// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class AuthorsEntryNode(KdlNode node) : NodeBase(node)
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        AuthorsNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "email", NodeKdlValue<KdlString>.Optional },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string AuthorName => Node.Name;

    public string? AuthorEmail => GetStringValue("email");
}
