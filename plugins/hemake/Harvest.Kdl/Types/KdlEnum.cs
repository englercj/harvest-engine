// Copyright Chad Engler

using System.Diagnostics;

namespace Harvest.Kdl.Types;

/// <summary>
/// A <see cref="KdlValue"/> wrapper around an enum.
/// </summary>
[DebuggerDisplay("{Value}")]
public class KdlEnum(Enum value, string? type = null) : KdlValue<Enum>(value, type)
{
    protected override void WriteKdlValue(TextWriter writer, KdlWriteOptions options)
    {
        KdlUtils.WriteString(writer, Convert.ToString(Value) ?? "", options);
    }
}
