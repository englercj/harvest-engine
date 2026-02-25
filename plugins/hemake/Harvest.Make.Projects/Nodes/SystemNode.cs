// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class SystemNodeTraits : NodeBaseTraits
{
    public override string Name => "system";

    public override IReadOnlyList<string> ValidScopes =>
    [
        ProjectNode.NodeTraits.Name,
    ];

    public override IReadOnlyList<NodeValueDef> ArgumentDefs =>
    [
        NodeValueDef_Enum<EPlatformSystem>.Required(EPlatformSystem.Windows),
    ];

    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "version", NodeValueDef_String.Optional() }
    };

    public override INode CreateNode(KdlNode node) => new SystemNode(node);
}

public class SystemNode(KdlNode node) : NodeBase<SystemNodeTraits>(node)
{
    public EPlatformSystem System => GetEnumValue<EPlatformSystem>(0);
    public string Version => GetResolvedVersion();

    private string GetResolvedVersion()
    {
        const string Latest = "latest";
        string version = GetValue<string>("version")?.ToLowerInvariant() ?? Latest;

        switch (System)
        {
            case EPlatformSystem.DotNet:
                return version == Latest ? "net9.0" : version;
            case EPlatformSystem.Linux:
                return version == Latest ? "" : version;
            case EPlatformSystem.WASM:
                return version == Latest ? "" : version;
            case EPlatformSystem.Windows:
                return version == Latest ? "10.0" : version;
        }

        throw new Exception($"Unknown platform system: {System}");
    }
}
