// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class GitHubNode(KdlNode node, INode? scope) : NodeBase(node, scope)
{
    public const string NodeName = "github";

    public static readonly IReadOnlyList<string> NodeScopes =
    [
        InstallNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "user", NodeKdlString.Required() },
        { "repo", NodeKdlString.Required() },
        { "ref", NodeKdlString.Required() },
    };

    public override string Name => NodeName;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string User => GetStringValue("user");
    public string Repo => GetStringValue("repo");
    public string Ref => GetStringValue("ref");
}
