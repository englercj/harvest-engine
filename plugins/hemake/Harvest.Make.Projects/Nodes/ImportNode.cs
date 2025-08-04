// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ImportNode(KdlNode node, INode? scope) : NodeBase<ImportNode>(node, scope)
{
    public static string NodeName => "import";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        InstallNode.NodeName,
        ModuleNode.NodeName,
        PluginNode.NodeName,
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_Path.Required(""),
    ];

    public string ImportPath => GetPathValue(0);
}
