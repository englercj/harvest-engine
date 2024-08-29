// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using System.Numerics;
using System.Security;

namespace Harvest.Make.Projects.Nodes;

public abstract class NodeBase(KdlNode node) : INode
{
    public bool IsExtensionNode => Node.Name.StartsWith('+');
    public virtual bool CanBeExtended => false;

    public abstract string Name { get; }
    public abstract IReadOnlyList<string> Scopes { get; }
    public abstract IReadOnlyList<NodeKdlValue> Arguments { get; }
    public abstract IReadOnlyDictionary<string, NodeKdlValue> Properties { get; }

    public KdlNode Node => node;

    private List<INode> _children = new();
    public IList<INode> Children => _children;

    public virtual bool CanHaveChildren => false;
    public virtual Type? ChildNodeType => null;

    public virtual Type? VariadicArgumentsType => null;

    public T? GetValue<T>(int index) where T : KdlValue
    {
        if (index >= Node.Arguments.Count)
        {
            return null;
        }

        return Node.Arguments[index] as T;
    }

    public T? GetValue<T>(string key) where T : KdlValue
    {
        return Node.Properties.TryGetValue(key, out KdlValue? value) ? value as T : null;
    }

    public bool? GetBoolValue(string key) => GetValue<KdlBool>(key)?.Value;
    public string? GetStringValue(string key) => GetValue<KdlString>(key)?.Value;
    public T? GetNumberValue<T>(string key) where T : struct, INumber<T>
    {
        return GetValue<KdlNumber<T>>(key)?.Value;
    }
    public T GetEnumValue<T>(string key, T defaultValue) where T : struct, Enum
    {
        return KdlEnumUtils.Parse(GetStringValue(key), defaultValue);
    }

    public bool? GetBoolValue(int index) => GetValue<KdlBool>(index)?.Value;
    public string? GetStringValue(int index) => GetValue<KdlString>(index)?.Value;
    public T? GetNumberValue<T>(int index) where T : struct, INumber<T>
    {
        return GetValue<KdlNumber<T>>(index)?.Value;
    }
    public T GetEnumValue<T>(int index, T defaultValue) where T : struct, Enum
    {
        return KdlEnumUtils.Parse(GetStringValue(index), defaultValue);
    }

    public NodeValidationResult Validate(INode? scope)
    {
        if (!CanBeExtended && IsExtensionNode)
        {
            return new NodeValidationResult(false, $"'{Name}' nodes cannot be extended.");
        }

        string nodeName = Node.Name;
        if (IsExtensionNode)
        {
            nodeName = nodeName[1..];
        }

        if (nodeName != Name)
        {
            throw new ArgumentException($"{GetType().Name} is meant for '{Name}' nodes, but got a '{nodeName}' node.");
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

        vr = ValidateChildren();
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

            // Special handling of `when` nodes. They have special child behavior.
            if (scope.Name == WhenNode.NodeName)
            {
                if (Name == WhenNode.NodeName)
                {
                    return new NodeValidationResult(false, $"'{Name}' nodes cannot be children of '{WhenNode.NodeName}' nodes.");
                }
            }
            else if (!Scopes.Contains(scope.Name))
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
            if (i < Arguments.Count)
            {
                if (!Node.Arguments[i].GetType().Equals(Arguments[i].ValueType))
                {
                    return new NodeValidationResult(false, $"'{Name}' node has incorrect value type in argument (index: {i}). Expected {Arguments[i].ValueType.Name} but got {Node.Arguments[i].GetType().Name}.");
                }
            }
            else if (VariadicArgumentsType is not null)
            {
                if (!Node.Arguments[i].GetType().Equals(VariadicArgumentsType))
                {
                    return new NodeValidationResult(false, $"'{Name}' node has incorrect value type in argument (index: {i}). Expected {VariadicArgumentsType.Name} but got {Node.Arguments[i].GetType().Name}.");
                }
            }
            else
            {
                return new NodeValidationResult(false, $"'{Name}' nodes cannot contain more than {Arguments.Count} arguments.");
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
        foreach (KeyValuePair<string, NodeKdlValue> pair in Properties)
        {
            if (Node.Properties.TryGetValue(pair.Key, out KdlValue? value))
            {
                if (pair.Value.ValueType != value.GetType())
                {
                    return new NodeValidationResult(false, $"'{Name}' node has incorrect value type in property '{pair.Key}'. Expected {pair.Value.ValueType.Name} but got {value.GetType().Name}.");
                }

                if (pair.Value.ValidValues.Count > 0)
                {
                    object? valid = pair.Value.ValidValues.FirstOrDefault(value.Equals);
                    if (valid is null)
                    {
                        return new NodeValidationResult(false, $"'{Name}' node has an invalid value in property '{pair.Key}'. Expected one of: {string.Join(", ", pair.Value.ValidValues)}.");
                    }
                }
            }
            else if (pair.Value.IsRequired)
            {
                return new NodeValidationResult(false, $"'{Name}' nodes must specify a '{pair.Key}' property.");
            }
        }

        return NodeValidationResult.Valid;
    }

    protected NodeValidationResult ValidateChildren()
    {
        if (ChildNodeType is not null)
        {
            foreach (KdlNode rawChild in Node.Children)
            {
                if (!rawChild.GetType().Equals(ChildNodeType))
                {
                    return new NodeValidationResult(false, $"Invalid child node type ('{rawChild.Name}') in '{Name}' node. Expected '{ChildNodeType.Name}'.");
                }
            }
        }

        return NodeValidationResult.Valid;
    }
}
