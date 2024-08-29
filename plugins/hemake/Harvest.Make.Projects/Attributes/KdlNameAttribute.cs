// Copyright Chad Engler

namespace Harvest.Make.Projects.Attributes;

[AttributeUsage(AttributeTargets.Field)]
public class KdlNameAttribute(string name) : Attribute
{
    public readonly string Name = name;
}
