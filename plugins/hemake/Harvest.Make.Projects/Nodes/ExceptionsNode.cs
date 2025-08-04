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

public class ExceptionsNode(KdlNode node, INode? scope) : NodeBase<ExceptionsNode>(node, scope)
{
    public static string NodeName => "exceptions";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ProjectNode.NodeName,
        ModuleNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_Enum<EExceptionsMode>.Required(EExceptionsMode.Default),
    ];

    public EExceptionsMode ExceptionsMode => GetEnumValue<EExceptionsMode>(0);
}
