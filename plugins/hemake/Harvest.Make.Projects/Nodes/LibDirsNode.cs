// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class LibDirsNode(KdlNode node, INode? scope) : NodeSetBase<LibDirsEntryNode>(node, scope)
{
    public const string NodeName = "lib_dirs";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
        ProjectNode.NodeName,
        PublicNode.NodeName,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "system", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public bool IsSystem => GetBoolValue("system");
}
