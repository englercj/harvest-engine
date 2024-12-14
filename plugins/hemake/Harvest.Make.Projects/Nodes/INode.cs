// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public interface INode
{
    public KdlNode Node { get; }
    public string Name { get; }

    public bool IsExtensionNode { get; }
    public bool CanBeExtended { get; }

    public IReadOnlyList<string> Scopes { get; }
    public IReadOnlyList<NodeKdlValue> Arguments { get; }
    public IReadOnlyDictionary<string, NodeKdlValue> Properties { get; }
    public INode? Scope { get; }
    public List<INode> Children { get; }
    public Type? ChildNodeType { get; }

    public NodeValidationResult Validate(INode? scope);
    public void MergeAndResolve(ProjectContext context, INode node);
}
