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

public class BuildEventNodeTraits : NodeBaseTraits
{
    public override string Name => "build_event";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ModuleNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EBuildEvent>.Required(EBuildEvent.Prebuild),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "message", NodeValueDef_String.Optional() },
    };
}

public class BuildEventNode(KdlNode node) : NodeBase<BuildEventNodeTraits>(node)
{
    public EBuildEvent EventName => GetEnumValue<EBuildEvent>(0);
    public string? Message => TryGetStringValue("message");
}
