// Copyright Chad Engler

using System.Diagnostics;

namespace Harvest.Kdl.Types;

/// <summary>
/// A <see cref="KdlValue"/> wrapper around a <see cref="string"/>.
/// </summary>
[DebuggerDisplay("{Value}")]
public class KdlString(string value, string? type = null) : KdlValue<string>(value, type)
{
    public static KdlString Empty => new("", null);

    protected override void WriteKdlValue(TextWriter writer, KdlWriteOptions options)
    {
        KdlUtils.WriteString(writer, Value, options);
    }
}
