// Copyright Chad Engler

namespace Harvest.Make.Attributes;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Method)]
public class CliCommandAttribute(string name, string? description = null) : Attribute
{
    public string Name { get; set; } = name;
    public string? Description { get; set; } = description;
    public string[] Aliases { get; set; } = [];

    public bool Hidden { get; set; } = false;
    public Type? Parent { get; set; }
}
