// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Microsoft.Extensions.FileSystemGlobbing;
using System.Diagnostics.CodeAnalysis;
using System.Numerics;
using System.Text.RegularExpressions;

namespace Harvest.Make.Projects.Nodes;

public abstract partial class NodeBase<TSelf>(KdlNode node, INode? scope) : INode where TSelf : INode
{
    public static IReadOnlyList<string> NodeValidScopes { get; } = [];
    public static IReadOnlyList<NodeValueDef> NodeArgumentDefs { get; } = [];
    public static IReadOnlyDictionary<string, NodeValueDef> NodePropertyDefs { get; } = new SortedDictionary<string, NodeValueDef>();
    public static ENodeDependencyInheritance NodeDependencyInheritance => ENodeDependencyInheritance.None;
    public static bool NodeCanBeExtended => false;

    public IReadOnlyList<string> ValidScopes => TSelf.NodeValidScopes;
    public IReadOnlyList<NodeValueDef> ArgumentDefs => TSelf.NodeArgumentDefs;
    public IReadOnlyDictionary<string, NodeValueDef> PropertyDefs => TSelf.NodePropertyDefs;
    public ENodeDependencyInheritance DependencyInheritance => TSelf.NodeDependencyInheritance;
    public bool CanBeExtended => TSelf.NodeCanBeExtended;

    public KdlNode Node => node;
    public INode? Scope => scope;

    public List<INode> Children { get; } = [];
    public virtual Type? ChildNodeType => null;

    protected T? TryGetClassValue<T>(int index) where T : class
    {
        if (index < Node.Arguments.Count)
        {
            if (Node.Arguments[index] is KdlValue<T> typedValue)
            {
                if (typedValue.Value is T result)
                {
                    return result;
                }
            }
        }

        if (index < ArgumentDefs.Count)
        {
            NodeValueDef argDef = ArgumentDefs[index];
            if (argDef.DefaultValue is T result)
            {
                return result;
            }
        }

        return null;
    }

    protected T? TryGetStructValue<T>(int index) where T : struct
    {
        if (index < Node.Arguments.Count)
        {
            if (Node.Arguments[index] is KdlValue<T> typedValue)
            {
                if (typedValue.Value is T result)
                {
                    return result;
                }
            }
        }

        if (index < ArgumentDefs.Count)
        {
            NodeValueDef argDef = ArgumentDefs[index];
            if (argDef.DefaultValue is T result)
            {
                return result;
            }
        }

        return null;
    }

    public bool? TryGetBoolValue(int index) => TryGetStructValue<bool>(index);
    public bool GetBoolValue(int index) => TryGetBoolValue(index) ?? throw new Exception($"No bool value specified for required argument index: {index}");

    public string? TryGetStringValue(int index) => TryGetClassValue<string>(index);
    public string GetStringValue(int index) => TryGetStringValue(index) ?? throw new Exception($"No string value specified for required argument index: {index}");

    public T? TryGetNumberValue<T>(int index) where T : struct, INumber<T> => TryGetStructValue<T>(index);
    public T GetNumberValue<T>(int index) where T : struct, INumber<T> => TryGetNumberValue<T>(index) ?? throw new Exception($"No number value specified for required argument index: {index}");

    public T? TryGetEnumValue<T>(int index) where T : struct, Enum
    {
        if (TryGetStringValue(index) is string strValue)
        {
            return KdlEnumUtils.TryParse(strValue, out T result) ? result : null;
        }

        if (index < ArgumentDefs.Count)
        {
            NodeValueDef argDef = ArgumentDefs[index];
            if (argDef.DefaultValue is T result)
            {
                return result;
            }
        }

        return null;
    }
    public T GetEnumValue<T>(int index) where T : struct, Enum => TryGetEnumValue<T>(index) ?? throw new Exception($"No enum value specified for required argument index: {index}");

    public string? TryGetPathValue(int index)
    {
        string? path = GetStringValue(index);
        return ResolvePath(path);
    }
    public string GetPathValue(int index) => TryGetPathValue(index) ?? throw new Exception($"No path value specified for required argument index: {index}");

    public IEnumerable<string> ExpandPathValue(int index)
    {
        string? path = GetStringValue(index);
        return ExpandPath(path);
    }

    protected T? TryGetClassValue<T>(string key) where T : class
    {
        if (Node.Properties.TryGetValue(key, out KdlValue? value))
        {
            if (value is KdlValue<T> typedValue)
            {
                if (typedValue.Value is T result)
                {
                    return result;
                }
            }
        }

        if (PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
        {
            if (valueDef.DefaultValue is T result)
            {
                return result;
            }
        }

        return null;
    }

    protected T? TryGetStructValue<T>(string key) where T : struct
    {
        if (Node.Properties.TryGetValue(key, out KdlValue? value))
        {
            if (value is KdlValue<T> typedValue)
            {
                if (typedValue.Value is T result)
                {
                    return result;
                }
            }
        }

        if (PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
        {
            if (valueDef.DefaultValue is T result)
            {
                return result;
            }
        }

        return null;
    }

    public bool? TryGetBoolValue(string key) => TryGetStructValue<bool>(key);
    public bool GetBoolValue(string key) => TryGetBoolValue(key) ?? throw new Exception($"No bool value specified for required property: {key}");

    public string? TryGetStringValue(string key) => TryGetClassValue<string>(key);
    public string GetStringValue(string key) => TryGetStringValue(key) ?? throw new Exception($"No string value specified for required property: {key}");

    public T? TryGetNumberValue<T>(string key) where T : struct, INumber<T> => TryGetStructValue<T>(key);
    public T GetNumberValue<T>(string key) where T : struct, INumber<T> => TryGetNumberValue<T>(key) ?? throw new Exception($"No number value specified for required property: {key}");

    public T? TryGetEnumValue<T>(string key) where T : struct, Enum
    {
        if (TryGetStringValue(key) is string strValue)
        {
            return KdlEnumUtils.TryParse(strValue, out T result) ? result : null;
        }

        if (PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
        {
            if (valueDef.DefaultValue is T result)
            {
                return result;
            }
        }

        return null;
    }
    public T GetEnumValue<T>(string key) where T : struct, Enum => TryGetEnumValue<T>(key) ?? throw new Exception($"No enum value specified for required property: {key}");

    public string? TryGetPathValue(string key)
    {
        string? path = TryGetStringValue(key);
        return ResolvePath(path);
    }
    public string GetPathValue(string key) => TryGetPathValue(key) ?? throw new Exception($"No path value specified for required property: {key}");

    public IEnumerable<string> ExpandPathValue(string key)
    {
        string? path = TryGetStringValue(key);
        return ExpandPath(path);
    }

    [return:NotNullIfNotNull(nameof(path))]
    protected string? ResolvePath(string? path)
    {
        if (path is not null && !Path.IsPathRooted(path))
        {
            if (!string.IsNullOrEmpty(Node.SourceInfo.FilePath))
            {
                string fileDir = Path.GetDirectoryName(Node.SourceInfo.FilePath)
                    ?? throw new Exception($"Failed to resolve path '{path}' from node '{Node.Name}' in file: {Node.SourceInfo.FilePath}");
                path = Path.GetFullPath(path, fileDir);
            }
            else
            {
                path = Path.GetFullPath(path);
            }
        }

        return path;
    }

    protected IEnumerable<string> ExpandPath(string? path)
    {
        if (path is not null)
        {
            string fileDir = Path.GetDirectoryName(Node.SourceInfo.FilePath)
                ?? throw new Exception($"Failed to expand path '{path}' from node '{Node.Name}' in file: {Node.SourceInfo.FilePath}");

            Matcher matcher = new();
            matcher.AddInclude(path);
            return matcher.GetResultsInFullPath(fileDir);
        }

        return [];
    }

    public virtual INode Clone()
    {
        KdlNode rawNode = Node.Clone();

        INode result = Activator.CreateInstance(GetType(), rawNode, Scope) as INode
            ?? throw new Exception($"Failed to clone node of type '{GetType().Name}'");

        foreach (INode child in Children)
        {
            INode clonedChild = child.Clone();
            result.Children.Add(clonedChild);
        }

        return result;
    }

    public virtual void Validate(INode? scope)
    {
        ValidateScope(scope);
        ValidateArguments();
        ValidateProperties();
        ValidateChildren();
    }

    protected virtual void ValidateScope(INode? scope)
    {
        if (ValidScopes.Count == 0)
        {
            if (scope is not null)
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes must be used at the root.");
            }
        }
        else
        {
            if (scope is null)
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes cannot be used at the root.");
            }

            // Special handling of `when` nodes. They have special child behavior.
            // TODO: Move this into an override in WhenNode.
            if (scope is WhenNode)
            {
                if (this is WhenNode)
                {
                    throw new NodeValidationException(this, $"'{Node.Name}' nodes cannot be a child of a '{WhenNode.NodeName}' node.");
                }
            }
            else if (!ValidScopes.Contains(scope.Node.Name))
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes cannot be a child of a '{scope.Node.Name}' node.");
            }
        }
    }

    protected virtual void ValidateArguments()
    {
        for (int i = 0; i < Node.Arguments.Count; ++i)
        {
            if (i < ArgumentDefs.Count)
            {
                KdlValue value = Node.Arguments[i];
                if (!value.GetType().Equals(ArgumentDefs[i].ValueType))
                {
                    throw new NodeValidationException(this, $"'{Node.Name}' node has incorrect value type in argument {i}: {value.GetType().Name}. Expected {ArgumentDefs[i].ValueType.Name}.");
                }

                if (ArgumentDefs[i].ValidValues.Count > 0)
                {
                    object? valid = ArgumentDefs[i].ValidValues.FirstOrDefault(value.Equals);
                    if (valid is null)
                    {
                        throw new NodeValidationException(this, $"'{Node.Name}' node has an invalid value in argument {i}: {value.GetValueString()}. Expected one of: {string.Join(", ", ArgumentDefs[i].ValidValues)}.");
                    }
                }
            }
            else
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes cannot contain more than {ArgumentDefs.Count} arguments.");
            }
        }

        for (int i = Node.Arguments.Count; i < ArgumentDefs.Count; ++i)
        {
            if (ArgumentDefs[i].IsRequired)
            {
                throw new NodeValidationException(this, $"'{Node.Name}' node is missing required argument (index: {i}).");
            }
        }
    }

    protected virtual void ValidateProperties()
    {
        foreach (KeyValuePair<string, NodeValueDef> pair in PropertyDefs)
        {
            if (Node.Properties.TryGetValue(pair.Key, out KdlValue? value))
            {
                if (!value.GetType().Equals(pair.Value.ValueType))
                {
                    throw new NodeValidationException(this, $"'{Node.Name}' node has incorrect value type in property '{pair.Key}': {value.GetType().Name}. Expected {pair.Value.ValueType.Name}.");
                }

                if (pair.Value.ValidValues.Count > 0)
                {
                    object? valid = pair.Value.ValidValues.FirstOrDefault(value.Equals);
                    if (valid is null)
                    {
                        throw new NodeValidationException(this, $"'{Node.Name}' node has an invalid value in property '{pair.Key}': {value.GetValueString()}. Expected one of: {string.Join(", ", pair.Value.ValidValues)}.");
                    }
                }
            }
            else if (pair.Value.IsRequired)
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes must specify a '{pair.Key}' property.");
            }
        }
    }

    protected virtual void ValidateChildren()
    {
        foreach (INode child in Children)
        {
            child.Validate(this);
        }
    }

    public virtual void MergeAndResolve(ProjectContext context, INode node)
    {
        if (!node.GetType().Equals(GetType()) || Node.Name != node.Node.Name)
        {
            throw new ArgumentException("Cannot merge nodes of different types.", nameof(node));
        }

        Node.SourceInfo = node.Node.SourceInfo;

        MergeAndResolveArguments(context, node);
        MergeAndResolveProperties(context, node);
        MergeAndResolveChildren(context, node);
    }

    protected virtual void MergeAndResolveArguments(ProjectContext context, INode node)
    {
        for (int i = 0; i < node.Node.Arguments.Count; ++i)
        {
            KdlValue value = node.Node.Arguments[i];

            if (value is KdlString valueStr)
            {
                string resolvedValue = context.ProjectService.TokenReplacer.ReplaceTokens(context, node.Node, valueStr.Value);
                AddOrSetArgument(i, new KdlString(resolvedValue, valueStr.Type));
            }
            else
            {
                AddOrSetArgument(i, value);
            }
        }
    }

    protected virtual void MergeAndResolveProperties(ProjectContext context, INode node)
    {
        foreach ((string key, KdlValue value) in node.Node.Properties)
        {
            if (value is KdlString valueStr)
            {
                string resolvedValue = context.ProjectService.TokenReplacer.ReplaceTokens(context, node.Node, valueStr.Value);
                Node.Properties[key] = new KdlString(resolvedValue, valueStr.Type);
            }
            else
            {
                Node.Properties[key] = value;
            }
        }
    }

    protected virtual void MergeAndResolveChildren(ProjectContext context, INode node)
    {
        foreach (INode child in node.Children)
        {
            Children.Add(child);
        }
    }

    protected virtual void AddOrSetArgument(int index, KdlValue value)
    {
        if (index < Node.Arguments.Count)
        {
            Node.Arguments[index] = value;
        }
        else if (index == Node.Arguments.Count)
        {
            Node.Arguments.Add(value);
        }
        else
        {
            throw new Exception($"Cannot add argument at index {index} to node '{Node.Name}'. Invalid arguments array.");
        }
    }
}
