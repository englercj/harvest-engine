// Copyright Chad Engler

using Harvest.Kdl;
using System.Reflection;
using System.Xml.Linq;

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
    public static virtual string NodeName => throw new NotImplementedException();
    public static virtual IReadOnlyList<string> NodeValidScopes => throw new NotImplementedException();
    public static virtual IReadOnlyList<NodeValueDef> NodeArgumentDefs => throw new NotImplementedException();
    public static virtual IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs => throw new NotImplementedException();
    public static virtual ENodeDependencyInheritance NodeDependencyInheritance => throw new NotImplementedException();
    public static virtual bool NodeCanBeExtended => throw new NotImplementedException();

    public IReadOnlyList<string> ValidScopes { get; }
    public IReadOnlyList<NodeValueDef> ArgumentDefs { get; }
    public IReadOnlyDictionary<string, NodeValueDef> PropertyDefs { get; }
    public ENodeDependencyInheritance DependencyInheritance { get; }
    public bool CanBeExtended { get; }

    public KdlNode Node { get; }
    public INode? Scope { get; }

    public List<INode> Children { get; }
    public Type? ChildNodeType { get; }

    public INode Clone();

    public void Validate(INode? scope);
    public void MergeAndResolve(ProjectContext context, INode node);
}
