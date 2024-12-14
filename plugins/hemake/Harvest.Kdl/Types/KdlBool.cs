// Copyright Chad Engler

using System.Diagnostics;

namespace Harvest.Kdl.Types;

/// <summary>
/// A <see cref="KdlValue"/> wrapper around a <see cref="bool"/>.
/// </summary>
[DebuggerDisplay("{Value}")]
public class KdlBool(bool value, string? type = null) : KdlValue<bool>(value, type)
{
    public static KdlBool True { get; } = new KdlBool(true);
    public static KdlBool False { get; } = new KdlBool(false);

    protected override void WriteKdlValue(TextWriter writer, KdlWriteOptions options)
    {
        writer.Write(Value ? "true" : "false");
    }
}
