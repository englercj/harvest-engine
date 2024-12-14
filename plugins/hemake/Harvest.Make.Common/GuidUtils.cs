// Copyright Chad Engler

using System.Security.Cryptography;
using System.Text;

namespace Harvest.Make;

public static class GuidUtils
{
    private static void SwapBytes(byte[] bytes, int left, int right)
    {
        byte temp = bytes[left];
        bytes[left] = bytes[right];
        bytes[right] = temp;
    }

    private static void SwapEndianness(byte[] bytes)
    {
        SwapBytes(bytes, 0, 3);
        SwapBytes(bytes, 1, 2);
        SwapBytes(bytes, 4, 5);
        SwapBytes(bytes, 6, 7);
    }

    public static Guid CreateV5(Guid namespaceId, string name)
    {
        if (string.IsNullOrEmpty(name))
            throw new ArgumentNullException("name", "The name parameter cannot be empty or null");

        byte[] nameBytes = Encoding.UTF8.GetBytes(name);
        byte[] namespaceBytes = namespaceId.ToByteArray();

        if (BitConverter.IsLittleEndian)
        {
            SwapEndianness(namespaceBytes);
        }

        byte[] hash = SHA1.HashData(namespaceBytes.Concat(nameBytes).ToArray());


        if (BitConverter.IsLittleEndian)
        {
            SwapEndianness(hash);
        }

        byte[] result = new byte[16];
        Array.Copy(hash, result, 16);

        // set version to 5
        result[BitConverter.IsLittleEndian ? 7 : 6] &= 0x0F;
        result[BitConverter.IsLittleEndian ? 7 : 6] |= 0x50;

        // set variant to RFC 4122
        result[8] &= 0x3F;
        result[8] |= 0x80;

        return new Guid(result);
    }
}
