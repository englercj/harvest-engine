// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class ConfigurationNodeTraits : NodeBaseTraits
{
    public override string Name => "configuration";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public override INode CreateNode(KdlNode node) => new ConfigurationNode(node);
}

public class ConfigurationNode(KdlNode node) : NodeBase<ConfigurationNodeTraits>(node)
{
    public string ConfigName => GetValue<string>(0);
}
