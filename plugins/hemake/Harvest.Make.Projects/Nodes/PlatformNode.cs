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

public class PlatformNodeTraits : NodeBaseTraits
{
    public override string Name => "platform";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "arch", NodeValueDef_Enum<EPlatformArch>.Required(EPlatformArch.X86_64) },
        { "system", NodeValueDef_Enum<EPlatformSystem>.Required(GetHostPlatform()) },
        { "toolset", NodeValueDef_Enum<EToolset>.Optional() },
        { "default", NodeValueDef_Bool.Optional(false) },
    };

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
}

public class PlatformNode(KdlNode node, INode? scope) : NodeBase<PlatformNodeTraits>(node, scope)
{
    public string PlatformName => GetStringValue(0);
    public EPlatformArch Arch => GetEnumValue<EPlatformArch>("arch");
    public EPlatformSystem System => GetEnumValue<EPlatformSystem>("system");
    public EToolset Toolset => TryGetEnumValue<EToolset>("toolset") ?? GuessToolset();
    public bool IsDefault => GetBoolValue("default");

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

    public override void Validate(INode? scope)
    {
        base.Validate(scope);

        if (Arch == EPlatformArch.Any && System != EPlatformSystem.DotNet)
        {
            throw new NodeValidationException(this, "Platform arch cannot be 'any' unless system is also 'dotnet'");
        }

        if (Arch == EPlatformArch.WASM32 && System != EPlatformSystem.WASM)
        {
            throw new NodeValidationException(this, "Platform arch cannot be 'wasm32' unless system is also 'wasm'");
        }

        if (System == EPlatformSystem.WASM && Arch != EPlatformArch.WASM32)
        {
            throw new NodeValidationException(this, "Platform system cannot be 'wasm' unless arch is also 'wasm32'");
        }
    }
}
