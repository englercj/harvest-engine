// Copyright Chad Engler

using Harvest.Kdl;
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

public class DependenciesEntryNode(KdlNode node, INode? scope) : NodeBase(node, scope), IEquatable<DependenciesEntryNode>
{
    public static readonly IReadOnlyList<string> NodeScopes =
    [
        DependenciesNode.NodeName,
    ];

    public static readonly IReadOnlyList<NodeKdlValue> NodeArguments =
    [
    ];

    public static readonly IReadOnlyDictionary<string, NodeKdlValue> NodeProperties = new SortedDictionary<string, NodeKdlValue>()
    {
        { "kind", NodeKdlEnum<EDependencyKind>.Optional(EDependencyKind.Default) },
        { "whole_archive", NodeKdlBool.Optional(false) },
    };

    public override string Name => Node.Name;
    public override IReadOnlyList<string> Scopes => NodeScopes;
    public override IReadOnlyList<NodeKdlValue> Arguments => NodeArguments;
    public override IReadOnlyDictionary<string, NodeKdlValue> Properties => NodeProperties;

    public string DependencyName => Node.Name;
    public EDependencyKind Kind => GetEnumValue<EDependencyKind>("kind");
    public bool WholeArchive => GetBoolValue("whole_archive");

    public override int GetHashCode() => HashCode.Combine(DependencyName, Kind, WholeArchive);

    public override bool Equals(object? other) => Equals(other as DependenciesEntryNode);

    public bool Equals(DependenciesEntryNode? other)
    {
        if (other is null)
        {
            return false;
        }

        if (ReferenceEquals(this, other))
        {
            return true;
        }

        if (GetType() != other.GetType())
        {
            return false;
        }

        return DependencyName == other.DependencyName
            && Kind == other.Kind
            && WholeArchive == other.WholeArchive;
    }
}
