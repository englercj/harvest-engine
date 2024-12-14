// Copyright Chad Engler

using Harvest.Make.Utils;
using System.CommandLine;
using System.Runtime.CompilerServices;

namespace Harvest.Make.Attributes;

[AttributeUsage(AttributeTargets.Property)]
public class CliArgumentAttribute([CallerMemberName] string? targetName = null) : Attribute
{
    public string Name { get; set; } = targetName is not null ? targetName.ToKebabCase() : throw new ArgumentNullException(nameof(targetName));
    public string Description { get; set; } = "";

    public bool IsHidden { get; set; } = false;
    public bool IsRequired { get; set; } = false;
}
