// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EExceptionsMode
{
    [KdlName("default")] Default,
    [KdlName("on")] On,
    [KdlName("off")] Off,
    [KdlName("seh")] SEH,
}

public class ExceptionsNodeTraits : NodeBaseTraits
{
    public override string Name => "exceptions";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
        ModuleNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EExceptionsMode>.Required(EExceptionsMode.Default),
    ];
}

public class ExceptionsNode(KdlNode node, INode? scope) : NodeBase<ExceptionsNodeTraits>(node, scope)
{
    public EExceptionsMode ExceptionsMode => GetEnumValue<EExceptionsMode>(0);
}
