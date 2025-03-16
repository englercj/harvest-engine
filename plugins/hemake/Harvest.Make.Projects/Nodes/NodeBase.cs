// Copyright Chad Engler

using Harvest.Kdl;
using Harvest.Kdl.Types;
using Microsoft.Extensions.FileSystemGlobbing;
using System.Diagnostics.CodeAnalysis;
using System.Numerics;
using System.Text.RegularExpressions;

namespace Harvest.Make.Projects.Nodes;

public abstract class NodeBase(KdlNode node, INode? scope) : INode
{
    public bool IsExtensionNode => Node.Name.StartsWith('+');
    public virtual bool CanBeExtended => false;

    public abstract string Name { get; }
    public abstract IReadOnlyList<string> Scopes { get; }
    public abstract IReadOnlyList<NodeKdlValue> Arguments { get; }
    public abstract IReadOnlyDictionary<string, NodeKdlValue> Properties { get; }
    public virtual ENodeDependencyInheritance DependencyInheritance => ENodeDependencyInheritance.None;

    public KdlNode Node => node;
    public INode? Scope => scope;

    public List<INode> Children { get; } = [];

    public virtual bool CanHaveChildren => false;
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

        if (index < Arguments.Count)
        {
            NodeKdlValue argDef = Arguments[index];
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

        if (index < Arguments.Count)
        {
            NodeKdlValue argDef = Arguments[index];
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
        return KdlEnumUtils.TryParse(GetStringValue(index), out T result) ? result : null;
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

        if (Properties.TryGetValue(key, out NodeKdlValue? valueDef))
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

        if (Properties.TryGetValue(key, out NodeKdlValue? valueDef))
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
        return KdlEnumUtils.TryParse(TryGetStringValue(key), out T result) ? result : null;
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
            if (!string.IsNullOrEmpty(Node.SourceInfo.FileName))
            {
                string fileDir = Path.GetDirectoryName(Node.SourceInfo.FileName)
                    ?? throw new Exception($"Failed to resolve path '{path}' from node '{Name}' in file: {Node.SourceInfo.FileName}");
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
            string fileDir = Path.GetDirectoryName(Node.SourceInfo.FileName)
                ?? throw new Exception($"Failed to expand path '{path}' from node '{Name}' in file: {Node.SourceInfo.FileName}");

            Matcher matcher = new();
            matcher.AddInclude(path);
            return matcher.GetResultsInFullPath(fileDir);
        }

        return [];
    }

    public virtual NodeValidationResult Validate(INode? scope)
    {
        if (!CanBeExtended && IsExtensionNode)
        {
            return NodeValidationResult.Error($"'{Name}' nodes cannot be extended.");
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

    protected virtual NodeValidationResult ValidateScope(INode? scope)
    {
        if (Scopes.Count == 0)
        {
            if (scope is not null)
            {
                return NodeValidationResult.Error($"'{Name}' nodes must be at the root.");
            }
        }
        else
        {
            if (scope is null)
            {
                return NodeValidationResult.Error($"'{Name}' nodes cannot be at the root.");
            }

            // Special handling of `when` nodes. They have special child behavior.
            if (scope.Name == WhenNode.NodeName)
            {
                if (Name == WhenNode.NodeName)
                {
                    return NodeValidationResult.Error($"'{Name}' nodes cannot be children of '{WhenNode.NodeName}' nodes.");
                }
            }
            else if (!Scopes.Contains(scope.Name))
            {
                return NodeValidationResult.Error($"'{Name}' nodes cannot be children of '{scope.Name}' nodes.");
            }
        }

        return NodeValidationResult.Valid;
    }

    protected virtual NodeValidationResult ValidateArguments()
    {
        for (int i = 0; i < Node.Arguments.Count; ++i)
        {
            if (i < Arguments.Count)
            {
                if (!Node.Arguments[i].GetType().Equals(Arguments[i].ValueType))
                {
                    return NodeValidationResult.Error($"'{Name}' node has incorrect value type in argument (index: {i}). Expected {Arguments[i].ValueType.Name} but got {Node.Arguments[i].GetType().Name}.");
                }
            }
            else
            {
                return NodeValidationResult.Error($"'{Name}' nodes cannot contain more than {Arguments.Count} arguments.");
            }
        }

        for (int i = Node.Arguments.Count; i < Arguments.Count; ++i)
        {
            if (Arguments[i].IsRequired)
            {
                return NodeValidationResult.Error($"'{Name}' node is missing required argument (index: {i}).");
            }
        }

        return NodeValidationResult.Valid;
    }

    protected virtual NodeValidationResult ValidateProperties()
    {
        foreach (KeyValuePair<string, NodeKdlValue> pair in Properties)
        {
            if (Node.Properties.TryGetValue(pair.Key, out KdlValue? value))
            {
                if (pair.Value.ValueType != value.GetType())
                {
                    return NodeValidationResult.Error($"'{Name}' node has incorrect value type in property '{pair.Key}'. Expected {pair.Value.ValueType.Name} but got {value.GetType().Name}.");
                }

                if (pair.Value.ValidValues.Count > 0)
                {
                    object? valid = pair.Value.ValidValues.FirstOrDefault(value.Equals);
                    if (valid is null)
                    {
                        return NodeValidationResult.Error($"'{Name}' node has an invalid value in property '{pair.Key}'. Expected one of: {string.Join(", ", pair.Value.ValidValues)}.");
                    }
                }
            }
            else if (pair.Value.IsRequired)
            {
                return NodeValidationResult.Error($"'{Name}' nodes must specify a '{pair.Key}' property.");
            }
        }

        return NodeValidationResult.Valid;
    }

    protected virtual NodeValidationResult ValidateChildren()
    {
        if (ChildNodeType is not null)
        {
            foreach (KdlNode rawChild in Node.Children)
            {
                if (!rawChild.GetType().Equals(ChildNodeType))
                {
                    return NodeValidationResult.Error($"Invalid child node type ('{rawChild.Name}') in '{Name}' node. Expected '{ChildNodeType.Name}'.");
                }
            }
        }

        return NodeValidationResult.Valid;
    }

    public void MergeAndResolve(ProjectContext context, INode node)
    {
        if (!node.GetType().Equals(GetType()) || Node.Name != node.Node.Name)
        {
            throw new Exception("Cannot merge nodes of different types.");
        }

        Node.SourceInfo = node.Node.SourceInfo;

        MergeAndResolveProperties(context, node);
        MergeAndResolveArguments(context, node);
        MergeAndResolveChildren(context, node);
    }

    protected virtual void MergeAndResolveProperties(ProjectContext context, INode node)
    {
        foreach ((string key, KdlValue value) in node.Node.Properties)
        {
            if (value is KdlString valueStr)
            {
                string resolvedValue = ReplaceTokens(context, valueStr.Value);
                Node.Properties.Add(key, new KdlString(resolvedValue, valueStr.Type));
            }
            else
            {
                Node.Properties.Add(key, value);
            }
        }
    }

    protected virtual void MergeAndResolveArguments(ProjectContext context, INode node)
    {
        for (int i = 0; i < node.Node.Arguments.Count; ++i)
        {
            KdlValue value = node.Node.Arguments[i];

            if (value is KdlString valueStr)
            {
                string resolvedValue = ReplaceTokens(context, valueStr.Value);
                AddOrSetArgument(i, new KdlString(resolvedValue, valueStr.Type));
            }
            else
            {
                AddOrSetArgument(i, value);
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
            throw new Exception($"Cannot add argument at index {index} to node '{Name}'. Invalid arguments array.");
        }
    }

    private const string _tokenRegexPattern = @"\$\{[^\}]+\}";

    [GeneratedRegex(_tokenRegexPattern, RegexOptions.Singleline)]
    private static partial Regex TokenRegex();

    protected string ReplaceTokens(ProjectContext projectContext, string input)
    {
        return TokenRegex().Replace(input, (match) =>
        {
            string token = match.Value[2..^1]; // ${token} -> token
            string[] contextParts = token.Split('.');

            if (contextParts.Length != 2)
            {
                throw new Exception($"Invalid token '{token}'. Expected format: 'context.property'.");
            }

            string[] transformerParts = contextParts[1].Split(':');

            string contextName = contextParts[0];
            string propertyName = transformerParts[0];
            string tokenValue = GetTokenValue(projectContext, token, contextName, propertyName);

            foreach (string transformer in transformerParts[1..])
            {
                tokenValue = transformer switch
                {
                    "lower" => tokenValue.ToLower(),
                    "upper" => tokenValue.ToUpper(),
                    "trim" => tokenValue.Trim(),
                    "dirname" => Path.GetDirectoryName(tokenValue) ?? "",
                    "basename" => Path.GetFileName(tokenValue) ?? "",
                    "extname" => Path.GetExtension(tokenValue)?.TrimStart('.') ?? "",
                    "extension" => Path.GetExtension(tokenValue) ?? "",
                    "noextension" => Path.ChangeExtension(tokenValue, null) ?? "",
                    _ => throw new Exception($"Invalid token '{token}'. Unknown transformer '{transformer}'."),
                };
            }

            return tokenValue;
        });
    }

    protected string GetTokenValue(ProjectContext projectContext, string token, string contextName, string propertyName)
    {
        if (contextName == ConfigurationNode.NodeName)
        {
            return propertyName switch
            {
                "name" => projectContext.Configuration,
                _ => throw new Exception($"Invalid token '{token}'. Unknown property '{propertyName}' on context 'configuration'."),
            };
        }

        if (contextName == PlatformNode.NodeName)
        {
            return propertyName switch
            {
                "name" => projectContext.Platform,
                "system" => KdlEnumUtils.GetName(projectContext.System),
                "arch" => KdlEnumUtils.GetName(projectContext.Arch),
                _ => throw new Exception($"Invalid token '{token}'. Unknown property '{propertyName}' on context 'platform'."),
            };
        }

        if (FindScopeWithName(contextName) is not INode nodeContext)
        {
            throw new Exception($"Invalid token '{token}'. Unknown context: '{contextName}'.");
        }

        if (propertyName.StartsWith("_arg"))
        {
            if (!int.TryParse(propertyName[4..], out int argIndex))
            {
                throw new Exception($"Invalid token '{token}'. Argument index must be an integer.");
            }

            if (nodeContext.Node.Arguments[argIndex].ToString() is string argValue)
            {
                return argValue;
            }
        }
        else if (nodeContext is ProjectNode projectNode)
        {
            if (propertyName == "name")
            {
                return projectNode.ProjectName;
            }
            else if (propertyName == "path")
            {
                return projectContext.ProjectPath;
            }
        }
        else if (nodeContext is PluginNode pluginNode)
        {
            if (propertyName == "name")
            {
                return pluginNode.PluginId;
            }
            else if (propertyName == "path")
            {
                return pluginNode.Node.SourceInfo.FileName;
            }
            else if (propertyName == "install_dir")
            {
                // TODO: read BuildOutputNode and return the correct path
            }
        }
        else if (nodeContext is ModuleNode moduleNode)
        {
            if (propertyName == "name")
            {
                return moduleNode.ModuleName;
            }
            else if (propertyName == "path")
            {
                return moduleNode.Node.SourceInfo.FileName;
            }
            else if (propertyName == "build_target")
            {
                // TODO: read BuildOutputNode and return the correct path
            }
            else if (propertyName == "link_target")
            {
                // TODO: read BuildOutputNode and return the correct path
            }
            else if (propertyName == "gen_dir")
            {
                // TODO: read BuildOutputNode and return the correct path
            }
        }

        if (nodeContext.Node.Properties.TryGetValue(propertyName, out KdlValue? value))
        {
            return value switch
            {
                KdlBool v => v.Value ? "true" : "false",
                KdlNumber<byte> v => v.Value.ToString(),
                KdlNumber<ushort> v => v.Value.ToString(),
                KdlNumber<uint> v => v.Value.ToString(),
                KdlNumber<ulong> v => v.Value.ToString(),
                KdlNumber<sbyte> v => v.Value.ToString(),
                KdlNumber<short> v => v.Value.ToString(),
                KdlNumber<int> v => v.Value.ToString(),
                KdlNumber<long> v => v.Value.ToString(),
                KdlNumber<nint> v => v.Value.ToString(),
                KdlNumber<nuint> v => v.Value.ToString(),
                KdlNumber<float> v => v.Value.ToString(),
                KdlNumber<double> v => v.Value.ToString(),
                KdlNumber<decimal> v => v.Value.ToString(),
                KdlString v => v.Value,
                _ => throw new Exception($"Invalid token '{token}'. Property '{propertyName}' has an unknown type on context '{contextName}'."),
            };
        }
        else
        {
            throw new Exception($"Invalid token '{token}'. Unknown property '{propertyName}' on context '{contextName}'.");
        }
    }

    protected INode? FindScopeWithName(string name)
    {
        INode? scope = this;
        while (scope is not null)
        {
            if (scope.Name == name)
            {
                return scope;
            }

            scope = scope.Scope;
        }

        return null;
    }
}
