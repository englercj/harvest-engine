// Copyright Chad Engler

namespace Harvest.Make.Projects.Attributes;

[AttributeUsage(AttributeTargets.Field)]
internal class KdlNameAttribute(string name) : Attribute
{
    public readonly string Name = name;
}
