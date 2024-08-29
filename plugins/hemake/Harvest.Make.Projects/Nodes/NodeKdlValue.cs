// Copyright Chad Engler

namespace Harvest.Make.Projects.Nodes;

public class NodeKdlValue(Type valueType)
{
    public Type ValueType { get; set; } = valueType;
    public bool IsRequired { get; set; } = false;
    public List<object> ValidValues { get; set; } = [];
}

public class NodeKdlValue<T> : NodeKdlValue where T : class
{
    public static NodeKdlValue<T> Required => new() { IsRequired = true };
    public static NodeKdlValue<T> Optional => new() { IsRequired = false };

    public NodeKdlValue() : base(typeof(T)) { }
}
