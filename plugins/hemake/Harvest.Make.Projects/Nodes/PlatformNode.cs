// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Make.Projects.Attributes;
using System.Runtime.InteropServices;

namespace Harvest.Make.Projects.Nodes;

public enum EPlatformArch
{
    [KdlName("any")] Any,
    [KdlName("arm")] Arm,
    [KdlName("arm64")] Arm64,
    [KdlName("x86")] X86,
    [KdlName("x86_64")] X86_64,
    [KdlName("wasm32")] WASM32,
}

public enum EPlatformSystem
{
    [KdlName("dotnet")] DotNet,
    [KdlName("linux")] Linux,
    [KdlName("wasm")] WASM,
    [KdlName("windows")] Windows,
}

public class PlatformNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "platform";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlString.Required(),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "arch", NodeKdlEnum<EPlatformArch>.Required(EPlatformArch.X86_64) },
        { "system", NodeKdlEnum<EPlatformSystem>.Required(GetHostPlatform()) },
        { "toolset", NodeKdlEnum<EToolset>.Optional() },
        { "default", NodeKdlBool.Optional(false) },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string PlatformName => GetStringValue(0);
    public EPlatformArch Arch => GetEnumValue<EPlatformArch>("arch");
    public EPlatformSystem System => GetEnumValue<EPlatformSystem>("system");
    public EToolset Toolset => TryGetEnumValue<EToolset>("toolset") ?? GuessToolset();
    public bool IsDefault => GetBoolValue("default");

    public static EPlatformSystem GetHostPlatform()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return EPlatformSystem.Windows;
        }

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            return EPlatformSystem.Linux;
        }

        throw new Exception("Unsupported host platform.");
    }

    private EToolset GuessToolset()
    {
        switch (System)
        {
            case EPlatformSystem.DotNet:
                return EToolset.MSVC;
            case EPlatformSystem.Linux:
                return EToolset.GCC;
            case EPlatformSystem.WASM:
                return EToolset.Clang;
            case EPlatformSystem.Windows:
                return EToolset.MSVC;
        }

        throw new Exception($"Unknown platform system: {System}");
    }

    public override NodeValidationResult Validate(INode? scope)
    {
        NodeValidationResult vr = base.Validate(scope);
        if (!vr.IsValid)
        {
            return vr;
        }

        if (Arch == EPlatformArch.Any && System != EPlatformSystem.DotNet)
        {
            return NodeValidationResult.Error("Platform arch cannot be 'any' unless system is also 'dotnet'");
        }

        if (Arch == EPlatformArch.WASM32 && System != EPlatformSystem.WASM)
        {
            return NodeValidationResult.Error("Platform arch cannot be 'wasm32' unless system is also 'wasm'");
        }

        if (System == EPlatformSystem.WASM && Arch != EPlatformArch.WASM32)
        {
            return NodeValidationResult.Error("Platform system cannot be 'wasm' unless arch is also 'wasm32'");
        }

        return NodeValidationResult.Valid;
    }
}
