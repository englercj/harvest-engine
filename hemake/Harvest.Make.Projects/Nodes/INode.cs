// Copyright Chad Engler

using Harvest.Kdl;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Make.Projects.Nodes;

// Describes how a node is inherited through dependencies
[Flags]
public enum ENodeDependencyInheritance
{
    // Not inherited via dependencies
    None    = 0,
    // Inherited via content dependencies
    Content = (1 << 0),
    // Inherited via include dependencies
    Include = (1 << 1),
    // Inherited via link dependencies
    Link    = (1 << 2),
    // Inherited via order dependencies
    Order   = (1 << 3),

    All = Content | Include | Link | Order,
}

public interface INodeTraits
{
    public string Name { get; }
    public IReadOnlyList<string> ValidScopes { get; }
    public IReadOnlyList<NodeValueDef> ArgumentDefs { get; }
    public IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; }
    public ENodeDependencyInheritance DependencyInheritance { get; }
    public bool CanBeExtended { get; }
    public INodeTraits? ChildNodeTraits { get; }

    public INode CreateNode(KdlNode node);
    public static T CreateNode<T>(KdlNode node) where T : class, INode => (T)T.NodeTraits.CreateNode(node);

    public string? TryResolveToken(ProjectContext projectContext, KdlNode contextNode, string propertyName);
    public bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode);

    public void Validate(KdlNode node);

    public bool TryGetValue<T>(KdlNode node, int index, [MaybeNullWhen(false)] out T value);
    public bool TryGetEnumValue<T>(KdlNode node, int index, [MaybeNullWhen(false)] out T value) where T : struct, Enum;
    public T GetValue<T>(KdlNode node, int index);
    public T GetEnumValue<T>(KdlNode node, int index) where T : struct, Enum;

    public bool TryGetValue<T>(KdlNode node, string key, [MaybeNullWhen(false)] out T value);
    public bool TryGetEnumValue<T>(KdlNode node, string key, [MaybeNullWhen(false)] out T value) where T : struct, Enum;
    public T GetValue<T>(KdlNode node, string key);
    public T GetEnumValue<T>(KdlNode node, string key) where T : struct, Enum;
}

public interface INode
{
    public static virtual INodeTraits NodeTraits => throw new NotImplementedException();

    public INodeTraits Traits { get; }

    public KdlNode Node { get; }

    public bool HasValue(int index);
    public bool TryGetValue<T>(int index, [MaybeNullWhen(false)] out T value);
    public bool TryGetEnumValue<T>(int index, [MaybeNullWhen(false)] out T value) where T : struct, Enum;
    public T GetValue<T>(int index);
    public T GetEnumValue<T>(int index) where T : struct, Enum;

    public bool HasValue(string key);
    public bool TryGetValue<T>(string key, [MaybeNullWhen(false)] out T value);
    public bool TryGetEnumValue<T>(string key, [MaybeNullWhen(false)] out T value) where T : struct, Enum;
    public T GetValue<T>(string key);
    public T GetEnumValue<T>(string key) where T : struct, Enum;

    public void MergeNode(ProjectContext projectContext, KdlNode node);
}
