// Copyright Chad Engler

using System.Diagnostics;

namespace Harvest.Kdl.Types;

/// <summary>
/// A <see cref="KdlValue"/> wrapper around a <see cref="bool"/>.
/// </summary>
[DebuggerDisplay("{Value}")]
public class KdlBool : KdlValue<bool>
{
    public static KdlBool True { get; } = new KdlBool(true, null);
    public static KdlBool False { get; } = new KdlBool(false, null);

    public KdlBool(bool value, string? type = null) : base(value, type) { }

    protected override void WriteKdlValue(TextWriter writer, KdlWriteOptions options)
    {
        writer.Write(Value ? "true" : "false");
    }
}
