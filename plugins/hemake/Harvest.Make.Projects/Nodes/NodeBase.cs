// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Microsoft.Extensions.FileSystemGlobbing;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Numerics;

namespace Harvest.Make.Projects.Nodes;

public abstract class NodeBaseTraits : INodeTraits
{
    public virtual string Name => string.Empty;
    public virtual IReadOnlyList<string> ValidScopes => [];
    public virtual IReadOnlyList<NodeValueDef> ArgumentDefs => [];
    public virtual IReadOnlyDictionary<string, NodeValueDef> PropertyDefs => new SortedDictionary<string, NodeValueDef>();
    public virtual ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.None;
    public virtual bool CanBeExtended => false;
    public virtual Type? ChildNodeType => null;

    public virtual string? TryResolveToken(ProjectContext projectContext, KdlNode contextNode, string propertyName)
    {
        if (propertyName == "path")
        {
            return contextNode.SourceInfo.FilePath;
        }

        return null;
    }
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

    #region Arguments and Properties

    protected T? TryGetClassValue<T>(int index) where T : class
    {
        if (Node.TryGetArgumentValue(index, out T? value))
        {
            return value;
        }

        if (index < Traits.ArgumentDefs.Count)
        {
            NodeValueDef argDef = Traits.ArgumentDefs[index];
            if (argDef.DefaultValue is T result)
            {
                return result;
            }
        }

        return null;
    }

    protected T? TryGetStructValue<T>(int index) where T : struct
    {
        if (Node.TryGetArgumentValue(index, out T? value))
        {
            return value;
        }

        if (index < Traits.ArgumentDefs.Count)
        {
            NodeValueDef argDef = Traits.ArgumentDefs[index];
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

        if (index < Traits.ArgumentDefs.Count)
        {
            NodeValueDef argDef = Traits.ArgumentDefs[index];
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
        if (Node.TryGetPropertyValue(key, out T? value))
        {
            return value;
        }

        if (Traits.PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
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
        if (Node.TryGetPropertyValue(key, out T? value))
        {
            return value;
        }

        if (Traits.PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
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

        if (Traits.PropertyDefs.TryGetValue(key, out NodeValueDef? valueDef))
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

    #endregion

    #region Validation

    public virtual void Validate(INode? scope)
    {
        ValidateScope(scope);
        ValidateArguments();
        ValidateProperties();
        ValidateChildren();
    }

    protected virtual void ValidateScope(INode? scope)
    {
        if (Traits.ValidScopes.Count == 0)
        {
            if (scope is not null)
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes must be used at the root.");
            }
        }
        else
        {
            // When we're in a WhenNode, validate against the parent scope instead since WhenNodes
            // are allowed to contain any node that is valid in their parent scope.
            while (scope is WhenNode)
            {
                scope = scope.Scope;
            }

            if (scope is null)
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes cannot be used at the root.");
            }

            if (!Traits.ValidScopes.Contains(scope.Node.Name))
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes cannot be a child of a '{scope.Node.Name}' node.");
            }
        }
    }

    protected virtual void ValidateArguments()
    {
        for (int i = 0; i < Node.Arguments.Count; ++i)
        {
            if (i < Traits.ArgumentDefs.Count)
            {
                KdlValue value = Node.Arguments[i];
                if (!value.GetType().Equals(Traits.ArgumentDefs[i].ValueType))
                {
                    throw new NodeValidationException(this, $"'{Node.Name}' node has incorrect value type in argument {i}: {value.GetType().Name}. Expected {Traits.ArgumentDefs[i].ValueType.Name}.");
                }

                if (Traits.ArgumentDefs[i].ValidValues.Count > 0)
                {
                    object? valid = Traits.ArgumentDefs[i].ValidValues.FirstOrDefault(value.Equals);
                    if (valid is null)
                    {
                        throw new NodeValidationException(this, $"'{Node.Name}' node has an invalid value in argument {i}: {value.GetValueString()}. Expected one of: {string.Join(", ", Traits.ArgumentDefs[i].ValidValues)}.");
                    }
                }
            }
            else
            {
                throw new NodeValidationException(this, $"'{Node.Name}' nodes cannot contain more than {Traits.ArgumentDefs.Count} arguments.");
            }
        }

        for (int i = Node.Arguments.Count; i < Traits.ArgumentDefs.Count; ++i)
        {
            if (Traits.ArgumentDefs[i].IsRequired)
            {
                throw new NodeValidationException(this, $"'{Node.Name}' node is missing required argument (index: {i}).");
            }
        }
    }

    protected virtual void ValidateProperties()
    {
        foreach (KeyValuePair<string, NodeValueDef> pair in Traits.PropertyDefs)
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

    #endregion

    // TODO: Only FilesNode & NodeSetBase override this, can I do it via traits instead?
    public virtual void MergeAndResolve(ProjectContext projectContext, INode node)
    {
        if (!node.GetType().Equals(GetType()) || Node.Name != node.Node.Name)
        {
            throw new ArgumentException("Cannot merge nodes of different types.", nameof(node));
        }

        Node.SourceInfo = node.Node.SourceInfo;

        MergeAndResolveArguments(projectContext, node);
        MergeAndResolveProperties(projectContext, node);
        MergeAndResolveChildren(projectContext, node);
    }

    // TODO: Only overridden by the BuildOutputs node currently
    public virtual void ResolveDefaults(ProjectContext projectContext)
    {
        // default the source info to the project file
        Node.SourceInfo = new KdlSourceInfo(projectContext.ProjectService.ProjectPath, 0, 0);
    }

    // TODO: No one overrides this currently
    protected virtual void MergeAndResolveArguments(ProjectContext projectContext, INode node)
    {
        for (int i = 0; i < node.Node.Arguments.Count; ++i)
        {
            KdlValue value = node.Node.Arguments[i];

            if (value is KdlString valueStr)
            {
                string resolvedValue = projectContext.ProjectService.TokenReplacer.ReplaceTokens(projectContext, node.Node, valueStr.Value);
                AddOrSetArgument(i, new KdlString(resolvedValue, valueStr.Type));
            }
            else
            {
                AddOrSetArgument(i, value);
            }
        }
    }

    // TODO: No one overrides this currently
    protected virtual void MergeAndResolveProperties(ProjectContext projectContext, INode node)
    {
        foreach ((string key, KdlValue value) in node.Node.Properties)
        {
            if (value is KdlString valueStr)
            {
                string resolvedValue = projectContext.ProjectService.TokenReplacer.ReplaceTokens(projectContext, node.Node, valueStr.Value);
                Node.Properties[key] = new KdlString(resolvedValue, valueStr.Type);
            }
            else
            {
                Node.Properties[key] = value;
            }
        }
    }

    // TODO: Only FilesNode & NodeSetBase override this, can I do it via traits instead?
    protected virtual void MergeAndResolveChildren(ProjectContext projectContext, INode node)
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
