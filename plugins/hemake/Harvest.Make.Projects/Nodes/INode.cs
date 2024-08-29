// Copyright Chad Engler

using Harvest.Kdl;
using System.Numerics;

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
    public IList<INode> Children { get; }
    public Type? ChildNodeType { get; }
    public Type? VariadicArgumentsType { get; }

    public T? GetValue<T>(int index) where T : KdlValue;
    public T? GetValue<T>(string key) where T : KdlValue;

    public bool? GetBoolValue(int index);
    public string? GetStringValue(int index);
    public T? GetNumberValue<T>(int index) where T : struct, INumber<T>;

    public bool? GetBoolValue(string key);
    public string? GetStringValue(string key);
    public T? GetNumberValue<T>(string key) where T : struct, INumber<T>;

    public NodeValidationResult Validate(INode? scope);
}
