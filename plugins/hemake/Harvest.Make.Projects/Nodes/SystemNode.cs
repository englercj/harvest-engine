// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class SystemNode(KdlNode node, INode? scope) : NodeBase<SystemNode>(node, scope)
{
    public static string NodeName => "system";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        ProjectNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_Enum<EPlatformSystem>.Required(EPlatformSystem.Windows),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "version", NodeValueDef_String.Optional() }
    };

    public EPlatformSystem System => GetEnumValue<EPlatformSystem>(0);
    public string Version => GetResolvedVersion();

    private string GetResolvedVersion()
    {
        const string Latest = "latest";
        string version = GetStringValue("version")?.ToLowerInvariant() ?? Latest;

        switch (System)
        {
            case EPlatformSystem.DotNet:
                return version == Latest ? "net8.0" : version;
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
