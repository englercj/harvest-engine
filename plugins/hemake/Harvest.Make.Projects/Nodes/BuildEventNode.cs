// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EBuildEvent
{
    [KdlName("prebuild")] Prebuild,
    [KdlName("build")] Build,
    [KdlName("postbuild")] Postbuild,
    [KdlName("prelink")] Prelink,
    [KdlName("postlink")] Postlink,
    [KdlName("clean")] Clean,
}

public class BuildEventNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "build_event";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ModuleNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EBuildEvent>.Required(EBuildEvent.Prebuild),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "message", NodeKdlString.Optional() },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EBuildEvent EventName => GetEnumValue<EBuildEvent>(0);
    public string? Message => TryGetStringValue("message");
}
