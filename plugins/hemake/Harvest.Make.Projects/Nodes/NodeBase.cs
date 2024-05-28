// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects.Nodes;

public abstract class NodeBase(KdlNode node) : INode
{
    public abstract string Name { get; }
    public abstract IReadOnlyList<string> Scopes { get; }
    public abstract IReadOnlyList<NodeArgument> Arguments { get; }
    public abstract IReadOnlyDictionary<string, NodeProperty> Properties { get; }

    public KdlNode Node => node;

    private List<INode> _children = new();
    public IList<INode> Children => _children;

    public NodeValidationResult Validate(INode? scope)
    {
        if (Node.Name != Name)
        {
            throw new ArgumentException($"{GetType().Name} is meant for '{Name}' nodes, but got a '{Node.Name}' node.");
        }

        NodeValidationResult vr = ValidateScope(scope);
        if (!vr.IsValid)
        {
            return vr;
        }

        vr = ValidateArguments();
        if (!vr.IsValid)
        {
            return vr;
        }

        vr = ValidateProperties();
        if (!vr.IsValid)
        {
            return vr;
        }

        return NodeValidationResult.Valid;
    }

    protected NodeValidationResult ValidateScope(INode? scope)
    {
        if (Scopes.Count == 0)
        {
            if (scope is not null)
            {
                return new NodeValidationResult(false, $"'{Name}' nodes must be at the root.");
            }
        }
        else
        {
            if (scope is null)
            {
                return new NodeValidationResult(false, $"'{Name}' nodes cannot be at the root.");
            }

            if (!Scopes.Contains(scope.Name))
            {
                return new NodeValidationResult(false, $"'{Name}' nodes cannot be children of '{scope.Name}' nodes.");
            }
        }

        return NodeValidationResult.Valid;
    }

    protected NodeValidationResult ValidateArguments()
    {
        for (int i = 0; i < Node.Arguments.Count; ++i)
        {
            if (i >= Arguments.Count)
            {
                return new NodeValidationResult(false, $"'{Name}' nodes cannot contain more than {Arguments.Count} arguments.");
            }

            NodeArgument argDesc = Arguments[i];
            if (argDesc.ValueType != Node.Arguments[i].GetType())
            {
                return new NodeValidationResult(false, $"'{Name}' node has incorrect value type in argument (index: {i}). Expected {argDesc.ValueType.Name} but got {Node.Arguments[i].GetType().Name}.");
            }
        }

        for (int i = Node.Arguments.Count; i < Arguments.Count; ++i)
        {
            if (Arguments[i].IsRequired)
            {
                return new NodeValidationResult(false, $"'{Name}' node is missing required argument (index: {i}).");
            }
        }

        return NodeValidationResult.Valid;
    }

    protected NodeValidationResult ValidateProperties()
    {
        foreach (KeyValuePair<string, NodeProperty> pair in Properties)
        {
            if (Node.Properties.TryGetValue(pair.Key, out KdlValue? value))
            {
                if (pair.Value.ValueType != value.GetType())
                {
                    return new NodeValidationResult(false, $"'{Name}' node has incorrect value type in property '{pair.Key}'. Expected {pair.Value.ValueType.Name} but got {value.GetType().Name}.");
                }
            }
            else if (pair.Value.IsRequired)
            {
                return new NodeValidationResult(false, $"'{Name}' nodes must specify a '{pair.Key}' property.");
            }
        }

        return NodeValidationResult.Valid;
    }
}
