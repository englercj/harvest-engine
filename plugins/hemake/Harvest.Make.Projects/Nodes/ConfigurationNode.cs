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
}

public class ConfigurationNode(KdlNode node, INode? scope) : NodeBase<ConfigurationNodeTraits>(node, scope)
{
    public string ConfigName => GetStringValue(0);
}
