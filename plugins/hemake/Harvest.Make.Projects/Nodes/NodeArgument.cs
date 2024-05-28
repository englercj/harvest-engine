// Copyright Chad Engler

namespace Harvest.Make.Projects.Nodes;

public class NodeArgument
{
    public Type ValueType { get; set; }
    public bool IsRequired { get; set; } = false;

    public NodeArgument(Type valueType)
    {
        ValueType = valueType;
    }
}

public class NodeArgument<T> : NodeArgument where T : class
{
    public static NodeArgument<T> Required => new() { IsRequired = true };
    public static NodeArgument<T> Optional => new() { IsRequired = false };

    public NodeArgument() : base(typeof(T)) { }
}
