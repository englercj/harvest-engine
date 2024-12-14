// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class NugetNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "nuget";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        InstallNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
        NodeKdlString.Required(),
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "version", NodeKdlString.Required() },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string PackageName => GetStringValue(0);
    public string PackageVersion => GetStringValue("version");
}
