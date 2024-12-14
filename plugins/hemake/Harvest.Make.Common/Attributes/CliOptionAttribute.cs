// Copyright Chad Engler

using Harvest.Make.Utils;
using System.Runtime.CompilerServices;

namespace Harvest.Make.Attributes;

[AttributeUsage(AttributeTargets.Property)]
public class CliOptionAttribute([CallerMemberName] string? targetName = null) : Attribute
{
    public string Name { get; set; } = targetName is not null ? targetName.ToKebabCase() : throw new ArgumentNullException(nameof(targetName));
    public string Description { get; set; } = "";
    public string[] Aliases { get; set; } = [];

    public bool IsHidden { get; set; } = false;
    public bool IsRequired { get; set; } = false;
}
