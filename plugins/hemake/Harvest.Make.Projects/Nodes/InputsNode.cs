// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InputsNode(KdlNode node, INode? scope) : NodeSetBase<InputsEntryNode>(node, scope)
{
    public const string NodeName = "inputs";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        BuildRuleNode.NodeName,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;
}
