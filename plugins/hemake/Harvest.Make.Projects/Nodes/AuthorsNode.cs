// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class AuthorsNode(KdlNode node, INode? scope) : NodeSetBase<AuthorsNode, AuthorsEntryNode>(node, scope)
{
    public static string NodeName => "authors";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        PluginNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
    };
}
