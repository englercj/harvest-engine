// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EExceptionsMode
{
    [KdlName("default")] Default,
    [KdlName("on")] On,
    [KdlName("off")] Off,
    [KdlName("seh")] SEH,
}

public class ExceptionsNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "exceptions";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
        ModuleNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EExceptionsMode>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EExceptionsMode ExceptionsMode => GetEnumValue(0, EExceptionsMode.Default);
}
