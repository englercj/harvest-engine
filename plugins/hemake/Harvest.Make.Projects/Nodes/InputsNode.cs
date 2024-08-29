// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class InputsNode(KdlNode node) : NodeSetBase<InputsEntryNode>(node)
{
    public const string NodeName = "inputs";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        BuildRuleNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlSetAction.Optional,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;
}
