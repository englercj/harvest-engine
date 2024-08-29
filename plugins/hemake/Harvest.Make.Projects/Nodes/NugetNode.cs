// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class NugetNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "nuget";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        InstallNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlValue<KdlString>.Required,
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "version", NodeKdlValue<KdlString>.Required },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? PackageName => GetStringValue(0);
    public string? PackageVersion => GetStringValue("version");
}
