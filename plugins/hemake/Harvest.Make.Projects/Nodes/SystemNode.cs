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
    public const string LatestVersion = "latest";

    public EPlatformSystem System => GetEnumValue<EPlatformSystem>(0);
    public string Version => GetResolvedVersion();
    public bool IsLatestVersion => IsVersionLatest(GetRawVersion());

    private string GetResolvedVersion()
    {
        string version = GetRawVersion();
        bool isLatest = IsVersionLatest(version);

        switch (System)
        {
            case EPlatformSystem.DotNet:
                return isLatest ? "net10.0" : version;
            case EPlatformSystem.Linux:
                return isLatest ? "" : version;
            case EPlatformSystem.WASM:
                return isLatest ? "" : version;
            case EPlatformSystem.Windows:
                return isLatest ? "10.0" : version;
        }

        throw new Exception($"Unknown platform system: {System}");
    }

    private string GetRawVersion()
    {
        return TryGetValue<string>("version", out string? value) ? value : LatestVersion;
    }

    private static bool IsVersionLatest(string version)
    {
        return string.Equals(version, LatestVersion, StringComparison.OrdinalIgnoreCase);
    }
}
