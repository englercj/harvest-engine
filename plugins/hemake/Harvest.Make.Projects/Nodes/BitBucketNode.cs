// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;

namespace Harvest.Make.Projects.Nodes;

public class BitBucketNode(KdlNode node) : NodeBase(node)
{
    public const string NodeName = "bitbucket";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        InstallNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "user", NodeKdlValue<KdlString>.Required },
        { "repo", NodeKdlValue<KdlString>.Required },
        { "ref", NodeKdlValue<KdlString>.Required },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string? User => GetStringValue("user");
    public string? Repo => GetStringValue("repo");
    public string? Ref => GetStringValue("ref");
}
