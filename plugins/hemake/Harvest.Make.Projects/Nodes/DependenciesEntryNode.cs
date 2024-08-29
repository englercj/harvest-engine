// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;

namespace Harvest.Make.Projects.Nodes;

public enum EDependencyKind
{
    [KdlName("default")] Default,
    [KdlName("file")] File,
    [KdlName("include")] Include,
    [KdlName("link")] Link,
    [KdlName("order")] Order,
    [KdlName("system")] System,
}

public class DependenciesEntryNode(KdlNode node) : NodeBase(node)
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        DependenciesNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new Dictionary<string, NodeKdlValue>()
    {
        { "kind", NodeKdlEnum<EDependencyKind>.Optional },
        { "whole_archive", NodeKdlValue<KdlBool>.Optional },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string DependencyName => Node.Name;

    public EDependencyKind Kind => GetEnumValue("kind", EDependencyKind.Default);

    public bool WholeArchive => GetBoolValue("whole_archive") ?? false;
}
