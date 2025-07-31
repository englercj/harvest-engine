// Copyright Chad Engler

using System.Diagnostics;

namespace Harvest.Kdl.Types;

/// <summary>
/// A <see cref="KdlValue"/> representing a null value.
/// </summary>
[DebuggerDisplay("null")]
public class KdlNull(string? type = null) : KdlValue(type)
{
    public static KdlNull Instance { get; } = new();

    protected override void WriteKdlValue(TextWriter writer, KdlWriteOptions options)
    {
        writer.Write("#null");
    }

    public override bool Equals(object? obj) => obj is KdlNull v && Type == v.Type;
    public override int GetHashCode() => Type is null ? 0 : Type.GetHashCode();
    public override string GetValueString() => "#null";
}
