// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class BitBucketNode(KdlNode node, INode? scope) : NodeBase<BitBucketNode>(node, scope)
{
    public static string NodeName => "bitbucket";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        InstallNode.NodeName,
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "user", NodeValueDef_String.Required() },
        { "repo", NodeValueDef_String.Required() },
        { "ref", NodeValueDef_String.Required() },
    };

    public string User => GetStringValue("user");
    public string Repo => GetStringValue("repo");
    public string Ref => GetStringValue("ref");
}
