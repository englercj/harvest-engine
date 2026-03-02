// Copyright Chad Engler

using System.Text;

namespace Harvest.Make;

public static class GuidExtensions
{
    public static Guid CreateV5(Guid namespaceId, string name)
    {
        ArgumentException.ThrowIfNullOrEmpty(name);

        // pre-allocate the correct amount of memory for the name bytes
        int nameByteCount = Encoding.UTF8.GetByteCount(name);
        byte[] nameBytes = new byte[16 + nameByteCount];
        namespaceId.TryWriteBytes(nameBytes);
        Encoding.UTF8.GetBytes(name, nameBytes.AsSpan(16..));

        if (BitConverter.IsLittleEndian)
        {
            SwapEndianness(nameBytes.AsSpan(..16));
        }

        SHA1Hash hash = SHA1Hash.HashData(nameBytes);

        Span<byte> guidBytes = stackalloc byte[16];
        hash.Data[..16].CopyTo(guidBytes);

        if (BitConverter.IsLittleEndian)
        {
            SwapEndianness(guidBytes);
        }

        // set version to 5
        guidBytes[BitConverter.IsLittleEndian ? 7 : 6] &= 0x0F;
        guidBytes[BitConverter.IsLittleEndian ? 7 : 6] |= 0x50;

        // set variant to RFC 4122
        guidBytes[8] &= 0x3F;
        guidBytes[8] |= 0x80;

        return new Guid(guidBytes);
    }

    [AggressiveInlining]
    private static void SwapBytes(Span<byte> bytes, int left, int right)
    {
        byte temp = bytes[left];
        bytes[left] = bytes[right];
        bytes[right] = temp;
    }

    [AggressiveInlining]
    private static void SwapEndianness(Span<byte> bytes)
    {
        SwapBytes(bytes, 0, 3);
        SwapBytes(bytes, 1, 2);
        SwapBytes(bytes, 4, 5);
        SwapBytes(bytes, 6, 7);
    }
}
