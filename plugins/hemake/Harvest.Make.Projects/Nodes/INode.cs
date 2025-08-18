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
    public Type? ChildNodeType { get; }

    public string? TryResolveToken(ProjectContext projectContext, KdlNode contextNode, string propertyName);
}

public interface INode
{
    public static virtual INodeTraits NodeTraits => throw new NotImplementedException();

    public INodeTraits Traits { get; }

    public KdlNode Node { get; }

    public void Validate(INode? scope);
    public void MergeAndResolve(ProjectContext projectContext, INode node);
    public void ResolveDefaults(ProjectContext projectContext);
}
