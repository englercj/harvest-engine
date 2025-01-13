// Copyright Chad Engler

using Harvest.Kdl;

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

public interface INode
{
    public KdlNode Node { get; }
    public string Name { get; }

    public bool IsExtensionNode { get; }
    public bool CanBeExtended { get; }
    public ENodeDependencyInheritance DependencyInheritance { get; }

    public IReadOnlyList<string> Scopes { get; }
    public IReadOnlyList<NodeKdlValue> Arguments { get; }
    public IReadOnlyDictionary<string, NodeKdlValue> Properties { get; }
    public INode? Scope { get; }
    public List<INode> Children { get; }
    public Type? ChildNodeType { get; }

    public NodeValidationResult Validate(INode? scope);
    public void MergeAndResolve(ProjectContext context, INode node);
}
