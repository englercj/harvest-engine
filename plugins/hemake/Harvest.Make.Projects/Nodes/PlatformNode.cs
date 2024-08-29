// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EPlatformArch
{
    [KdlName("x86")] X86,
    [KdlName("x86_64")] X86_64,
    [KdlName("arm")] Arm,
    [KdlName("arm64")] Arm64,
}

public enum EPlatformSystem
{
    [KdlName("linux")] Linux,
    [KdlName("wasm")] WASM,
    [KdlName("windows")] Windows,
}

public class PlatformNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "platform";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlValue<KdlString>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "arch", NodeKdlEnum<EPlatformArch>.Required },
        { "system", NodeKdlEnum<EPlatformSystem>.Required },
        { "toolset", NodeKdlValue<KdlString>.Optional },
        { "default", NodeKdlValue<KdlBool>.Optional },
        { "version", NodeKdlValue<KdlString>.Optional }
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public EPlatformArch Arch => GetEnumValue("arch", EPlatformArch.X86_64);
    public EPlatformSystem System => GetEnumValue("system", EPlatformSystem.Windows);
    public string? Toolset => GetStringValue("toolset");
    public bool IsDefault => GetBoolValue("default") ?? false;
    public string? Version => GetStringValue("version");
}
