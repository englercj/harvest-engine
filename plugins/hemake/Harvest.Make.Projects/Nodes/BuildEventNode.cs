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

public class BuildEventNode(KdlNode node, INode? scope) : NodeBase<BuildEventNode>(node, scope)
{
    public static string NodeName => "build_event";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ModuleNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_Enum<EBuildEvent>.Required(EBuildEvent.Prebuild),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "message", NodeValueDef_String.Optional() },
    };

    public EBuildEvent EventName => GetEnumValue<EBuildEvent>(0);
    public string? Message => TryGetStringValue("message");
}
