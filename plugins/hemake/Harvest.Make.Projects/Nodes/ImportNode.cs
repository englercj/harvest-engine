// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ImportNodeTraits : NodeBaseTraits
{
    public override string Name => "import";

    public override IReadOnlyList<string> ValidScopes =>
    [
        InstallNode.NodeTraits.Name,
        ModuleNode.NodeTraits.Name,
        PluginNode.NodeTraits.Name,
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Path.Required(""),
    ];

    public override INode CreateNode(KdlNode node) => new ImportNode(node);
}

public class ImportNode(KdlNode node) : NodeBase<ImportNodeTraits>(node)
{
    public string ImportPath => GetValue<string>(0);
}
