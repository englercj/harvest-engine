// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public class NugetNode(KdlNode node, INode? scope) : NodeBase<NugetNode>(node, scope)
{
    public static string NodeName => "nuget";

    public static new IReadOnlyList<string> NodeValidScopes =>
    [
        InstallNode.NodeName,
    ];

    public static new IReadOnlyList<NodeValueDef> NodeArgumentDefs =>
    [
        NodeValueDef_String.Required(),
    ];

    public static new IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "version", NodeValueDef_String.Required() },
    };

    public string PackageName => GetStringValue(0);
    public string PackageVersion => GetStringValue("version");
}
