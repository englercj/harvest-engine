using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Cryptography;

namespace Harvest.Common;

/// <summary>
/// Represents a SHA-1 hash value as a fixed-size, immutable structure.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
public readonly struct SHA1Hash : IEquatable<SHA1Hash>
{
    public const int HashSizeInBytes = SHA1.HashSizeInBytes;

    [InlineArray(HashSizeInBytes)] private struct DataArray { public byte Byte0; }

    private readonly DataArray _data;
    public ReadOnlySpan<byte> Data => MemoryMarshal.CreateReadOnlySpan(ref Unsafe.AsRef(in _data.Byte0), HashSizeInBytes);

    public SHA1Hash(byte[] hashBytes) : this(hashBytes.AsSpan(0, HashSizeInBytes))
    {}

    public SHA1Hash(ReadOnlySpan<byte> hashBytes)
    {
        if (hashBytes.Length != HashSizeInBytes)
        {
            throw new ArgumentException($"Must be {HashSizeInBytes} bytes", nameof(hashBytes));
        }

        hashBytes.CopyTo(_data);
    }

    public override string ToString() => Convert.ToHexStringLower(_data);

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool Equals(SHA1Hash other) => Data.SequenceEqual(other.Data);

    public override bool Equals(object? obj) => obj is SHA1Hash other && Equals(other);

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public override int GetHashCode() => MemoryMarshal.Read<int>(_data);

    public static bool operator ==(SHA1Hash left, SHA1Hash right) => left.Equals(right);
    public static bool operator !=(SHA1Hash left, SHA1Hash right) => !left.Equals(right);

    public byte this[int index] => _data[index];

    public static SHA1Hash HashData(ReadOnlySpan<byte> data)
    {
        Span<byte> buffer = stackalloc byte[HashSizeInBytes];
        SHA1.HashData(data, buffer);
        return new SHA1Hash(buffer);
    }
}
