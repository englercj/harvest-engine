// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;

namespace Harvest.Make.Projects.Nodes;

public abstract class NodeBaseTraits : INodeTraits
{
    public virtual string Name => string.Empty;
    public virtual IReadOnlyList<string> ValidScopes => [];
    public virtual IReadOnlyList<NodeValueDef> ArgumentDefs => [];
    public virtual IReadOnlyDictionary<string, NodeValueDef> PropertyDefs => new SortedDictionary<string, NodeValueDef>();
    public virtual ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.None;
    public virtual bool CanBeExtended => false;
    public virtual INodeTraits? ChildNodeTraits => null;

    public abstract INode CreateNode(KdlNode node);

    public virtual string? TryResolveToken(ProjectContext projectContext, KdlNode contextNode, string propertyName)
    {
        if (propertyName == "file_path")
        {
            return contextNode.SourceInfo.FilePath;
        }

        return null;
    }

    public virtual bool TryResolveChild(KdlNode target, KdlNode source, StringTokenReplacer replacer, NodeResolver resolver, out KdlNode? resolvedNode)
    {
        resolvedNode = null;
        return true;
    }

    #region Validation

    public virtual void Validate(KdlNode node)
    {
        Debug.Assert(node.Name == Name);

        ValidateScope(node);
        ValidateArguments(node);
        ValidateProperties(node);
    }

    protected virtual void ValidateScope(KdlNode node)
    {
        KdlNode? scope = node.Parent;

        if (ValidScopes.Count == 0)
        {
            if (scope is not null)
            {
                throw new NodeParseException(node, $"'{node.Name}' nodes must be used at the root.");
            }
        }
        else
        {
            // When we're in a WhenNode, validate against the parent scope instead since WhenNodes
            // are allowed to contain any node that is valid in their parent scope.
            while (scope is not null && scope.Name == WhenNode.NodeTraits.Name)
            {
                scope = scope.Parent;
            }

            if (scope is null)
            {
                throw new NodeParseException(node, $"'{node.Name}' nodes cannot be used at the root.");
            }

            if (!ValidScopes.Contains(scope.Name))
            {
                throw new NodeParseException(node, $"'{node.Name}' nodes cannot be a child of a '{scope.Name}' node.");
            }
        }
    }

    protected virtual void ValidateArguments(KdlNode node)
    {
        for (int i = 0; i < node.Arguments.Count; ++i)
        {
            if (i < ArgumentDefs.Count)
            {
                KdlValue value = node.Arguments[i];
                if (!value.GetType().Equals(ArgumentDefs[i].ValueType))
                {
                    throw new NodeParseException(node, $"'{node.Name}' node has incorrect value type in argument {i}: {value.GetType().Name}. Expected {ArgumentDefs[i].ValueType.Name}.");
                }

                if (ArgumentDefs[i].ValidValues.Count > 0)
                {
                    object? valid = ArgumentDefs[i].ValidValues.FirstOrDefault(value.Equals);
                    if (valid is null)
                    {
                        throw new NodeParseException(node, $"'{node.Name}' node has an invalid value in argument {i}: {value.GetValueString()}. Expected one of: {string.Join(", ", ArgumentDefs[i].ValidValues)}.");
                    }
                }
            }
            else
            {
                throw new NodeParseException(node, $"'{node.Name}' nodes cannot contain more than {ArgumentDefs.Count} arguments.");
            }
        }

        for (int i = node.Arguments.Count; i < ArgumentDefs.Count; ++i)
        {
            if (ArgumentDefs[i].IsRequired)
            {
                throw new NodeParseException(node, $"'{node.Name}' node is missing required argument (index: {i}).");
            }
        }
    }

    protected virtual void ValidateProperties(KdlNode node)
    {
        foreach (KeyValuePair<string, NodeValueDef> pair in PropertyDefs)
        {
            if (node.Properties.TryGetValue(pair.Key, out KdlValue? value))
            {
                if (!value.GetType().Equals(pair.Value.ValueType))
                {
                    throw new NodeParseException(node, $"'{node.Name}' node has incorrect value type in property '{pair.Key}': {value.GetType().Name}. Expected {pair.Value.ValueType.Name}.");
                }

                if (pair.Value.ValidValues.Count > 0)
                {
                    object? valid = pair.Value.ValidValues.FirstOrDefault(value.Equals);
                    if (valid is null)
                    {
                        throw new NodeParseException(node, $"'{node.Name}' node has an invalid value in property '{pair.Key}': {value.GetValueString()}. Expected one of: {string.Join(", ", pair.Value.ValidValues)}.");
                    }
                }
            }
            else if (pair.Value.IsRequired)
            {
                throw new NodeParseException(node, $"'{node.Name}' nodes must specify a '{pair.Key}' property.");
            }
        }
    }

    #endregion

    #region Argument Value Accessors

    public bool TryGetValue<T>(KdlNode node, int index, [MaybeNullWhen(false)] out T value)
    {
        Debug.Assert(node.Name == Name);

        if (node.TryGetValue(index, out value) && value is not null)
        {
            return true;
        }

        if (index > 0 && index < ArgumentDefs.Count)
        {
            NodeValueDef argDef = ArgumentDefs[index];
            if (argDef.DefaultValue is KdlValue<T> result)
            {
                value = result;
                return true;
            }
        }

        return false;
    }

    public bool TryGetEnumValue<T>(KdlNode node, int index, [MaybeNullWhen(false)] out T value) where T : struct, Enum
    {
        Debug.Assert(node.Name == Name);

        if (node.TryGetValue(index, out string? valueString) && valueString is not null)
        {
            return KdlEnumUtils.TryParse(valueString, out value);
        }

        if (index > 0 && index < ArgumentDefs.Count)
        {
            NodeValueDef argDef = ArgumentDefs[index];
            if (argDef.DefaultValue is KdlValue<T> result)
            {
                value = result;
                return true;
            }
        }

        value = default;
        return false;
    }

    public T GetValue<T>(KdlNode node, int index)
    {
        if (TryGetValue(node, index, out T? value) && value is not null)
        {
            return value;
        }
        throw new NodeParseException(node, $"Node does not have an argument at index {index} or the value is null.");
    }

    public T GetEnumValue<T>(KdlNode node, int index) where T : struct, Enum
    {
        if (TryGetEnumValue(node, index, out T value))
        {
            return value;
        }
        return default;
    }

    #endregion

    #region Property Value Accessors

    public bool TryGetValue<T>(KdlNode node, string key, [MaybeNullWhen(false)] out T value)
    {
        Debug.Assert(node.Name == Name);

        if (node.TryGetValue(key, out value))
        {
            return true;
        }

        if (PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
        {
            if (valueDef.DefaultValue is KdlValue<T> result)
            {
                value = result;
                return true;
            }
        }

        return false;
    }

    public bool TryGetEnumValue<T>(KdlNode node, string key, [MaybeNullWhen(false)] out T value) where T : struct, Enum
    {
        Debug.Assert(node.Name == Name);

        if (node.TryGetValue(key, out string? valueString) && valueString is not null)
        {
            return KdlEnumUtils.TryParse(valueString, out value);
        }

        if (PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
        {
            if (valueDef.DefaultValue is KdlString result)
            {
                return KdlEnumUtils.TryParse(result.Value, out value);
            }
        }

        value = default;
        return false;
    }

    public T GetValue<T>(KdlNode node, string key)
    {
        if (TryGetValue(node, key, out T? value) && value is not null)
        {
            return value;
        }
        throw new NodeParseException(node, $"Node does not have a property '{key}' or the value is null.");
    }

    public T GetEnumValue<T>(KdlNode node, string key) where T : struct, Enum
    {
        if (TryGetEnumValue(node, key, out T value))
        {
            return value;
        }
        return default;
    }

    #endregion
}

public abstract class NodeBase<TTraits> : INode
    where TTraits : NodeBaseTraits, new()
{
    private static readonly TTraits _nodeTraits = new();

    public static INodeTraits NodeTraits => _nodeTraits;

    public INodeTraits Traits => _nodeTraits;

    private readonly KdlNode _node;
    public KdlNode Node => _node;

    public NodeBase(KdlNode node)
    {
        Debug.Assert(string.IsNullOrEmpty(Traits.Name) || node.Name == Traits.Name,
            $"NodeBase<{typeof(TTraits).Name}> instantiated with incorrect KdlNode: {node.Name}. Expected: {Traits.Name}.");

        _node = node;
    }

    public bool HasValue(int index) => _node.HasValue(index);
    public bool TryGetValue<T>(int index, [MaybeNullWhen(false)] out T value) => _nodeTraits.TryGetValue(_node, index, out value);
    public bool TryGetEnumValue<T>(int index, [MaybeNullWhen(false)] out T value) where T : struct, Enum => _nodeTraits.TryGetEnumValue(_node, index, out value);
    public T GetValue<T>(int index) => _nodeTraits.GetValue<T>(_node, index);
    public T GetEnumValue<T>(int index) where T : struct, Enum => _nodeTraits.GetEnumValue<T>(_node, index);

    public bool HasValue(string key) => _node.HasValue(key);
    public bool TryGetValue<T>(string key, [MaybeNullWhen(false)] out T value) => _nodeTraits.TryGetValue(_node, key, out value);
    public bool TryGetEnumValue<T>(string key, [MaybeNullWhen(false)] out T value) where T : struct, Enum => _nodeTraits.TryGetEnumValue(_node, key, out value);
    public T GetValue<T>(string key) => _nodeTraits.GetValue<T>(_node, key);
    public T GetEnumValue<T>(string key) where T : struct, Enum => _nodeTraits.GetEnumValue<T>(_node, key);

    public virtual void MergeNode(ProjectContext projectContext, KdlNode node)
    {
        if (Node.Name != node.Name)
        {
            throw new ArgumentException("Cannot merge nodes of different types.", nameof(node));
        }

        node.CopyTo(Node, includeChildren: true);
    }
}
