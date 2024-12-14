// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class SystemNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "system";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        ProjectNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlEnum<EPlatformSystem>.Required(EPlatformSystem.Windows),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "version", NodeKdlString.Optional() }
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

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
