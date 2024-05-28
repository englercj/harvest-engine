// Copyright Chad Engler

using System.Diagnostics;

namespace Harvest.Kdl.Types;

/// <summary>
/// A <see cref="KdlValue"/> wrapper around a <see cref="string"/>.
/// </summary>
[DebuggerDisplay("{Value}")]
public class KdlString : KdlValue<string>
{
    public static KdlString Empty => new KdlString(string.Empty, null);

    public KdlString(string value, string? type = null) : base(value, type) { }

    protected override void WriteKdlValue(TextWriter writer, KdlWriteOptions options)
    {
        KdlUtils.WriteString(writer, Value, options);
    }
}
