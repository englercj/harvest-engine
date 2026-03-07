// Copyright Chad Engler

using System.Diagnostics.CodeAnalysis;

namespace Harvest.Kdl;

public class KdlSourceInfo(string? filePath, int line, int column) : IEquatable<KdlSourceInfo>
{
    public string? FilePath => filePath;
    public int Line => line;
    public int Column => column;

    public KdlSourceInfo() : this(null, 0, 0) { }

    public override bool Equals([NotNullWhen(true)] object? obj) => Equals(obj as KdlSourceInfo);
    public override int GetHashCode() => HashCode.Combine(Line, Column);
    public override string ToString() => $"KdlSourceInfo{{ FileName={FilePath}, Line={Line}, Column={Column} }}";
    public string ToErrorString() => $"{FilePath ?? ""}({Line},{Column})";

    public bool Equals([NotNullWhen(true)] KdlSourceInfo? other)
    {
        return other is not null
            && string.Equals(other.FilePath, FilePath, StringComparison.Ordinal)
            && other.Line == Line
            && other.Column == Column;
    }

    public static bool operator==(KdlSourceInfo? a, KdlSourceInfo? b) => a is null ? b is null : a.Equals(b);
    public static bool operator!=(KdlSourceInfo? a, KdlSourceInfo? b) => !(a == b);
}
