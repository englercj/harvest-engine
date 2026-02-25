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

public class DependenciesEntryNodeTraits : NodeSetEntryBaseTraits<DependenciesNode>
{
    public override IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>()
    {
        { "kind", NodeValueDef_Enum<EDependencyKind>.Optional(EDependencyKind.Default) },
        { "external", NodeValueDef_Bool.Optional(false) },
        { "whole_archive", NodeValueDef_Bool.Optional(false) },
    };

    public override void Validate(KdlNode node)
    {
        base.Validate(node);
        // TODO: Validate that the dependency exists in the project.
    }

    public override INode CreateNode(KdlNode node) => new DependenciesEntryNode(node);
}

public class DependenciesEntryNode(KdlNode node) : NodeSetEntryBase<DependenciesEntryNodeTraits, DependenciesNode>(node), IEquatable<DependenciesEntryNode>
{
    public string DependencyName => Kind == EDependencyKind.File ? ResolvePath(Node.Name) : Node.Name;
    public EDependencyKind Kind => GetEnumValue<EDependencyKind>("kind");
    public bool IsExternal => GetValue<bool>("external");
    public bool IsWholeArchive => GetValue<bool>("whole_archive");

    public override int GetHashCode() => HashCode.Combine(DependencyName, Kind, IsExternal, IsWholeArchive);

    public override bool Equals(object? other) => Equals(other as DependenciesEntryNode);

    public bool Equals(DependenciesEntryNode? entry)
    {
        if (entry is null)
        {
            return false;
        }

        if (ReferenceEquals(this, entry))
        {
            return true;
        }

        return DependencyName == entry.DependencyName
            && Kind == entry.Kind
            && IsExternal == entry.IsExternal
            && IsWholeArchive == entry.IsWholeArchive;
    }
}
