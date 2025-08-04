// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ArchiveNode(KdlNode node, INode? scope) : NodeBase<ArchiveNode>(node, scope), INode
{
    public static string NodeName => "archive";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        InstallNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "base", NodeValueDef_Path.Optional() },
    };

    public string Url => GetStringValue(0);
    public string? BasePath => TryGetPathValue("base");
}
