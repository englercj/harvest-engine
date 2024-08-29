// Copyright Chad Engler

using Harvest.Make.Utils;
using System.Runtime.CompilerServices;

namespace Harvest.Make.Attributes;

[AttributeUsage(AttributeTargets.Property)]
public class CliOptionAttribute([CallerMemberName] string? targetName = null) : Attribute
{
    public string Name { get; set; } = targetName is not null ? targetName.ToKebabCase() : throw new ArgumentNullException(nameof(targetName));
    public string ShortName { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string[] Aliases { get; set; } = [];

    public bool Hidden { get; set; } = false;
    public bool Required { get; set; } = false;
    public string[] AllowedValues { get; set; } = [];
}
