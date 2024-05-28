// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public interface INode
{
    public KdlNode Node { get; }
    public string Name { get; }
    public IReadOnlyList<string> Scopes { get; }
    public IReadOnlyList<NodeArgument> Arguments { get; }
    public IReadOnlyDictionary<string, NodeProperty> Properties { get; }
    public IList<INode> Children { get; }

    public NodeValidationResult Validate(INode? scope);
}
