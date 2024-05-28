// Copyright Chad Engler

namespace Harvest.Make.Projects.Nodes;

public class NodeProperty
{
    public Type ValueType { get; set; }
    public bool IsRequired { get; set; } = false;

    public NodeProperty(Type valueType)
    {
        ValueType = valueType;
    }
}

public class NodeProperty<T> : NodeProperty where T : class
{
    public static NodeProperty<T> Required => new() { IsRequired = true };
    public static NodeProperty<T> Optional => new() { IsRequired = false };

    public NodeProperty() : base(typeof(T)) { }
}
