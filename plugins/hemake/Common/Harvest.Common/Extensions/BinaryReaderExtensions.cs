namespace Luna.Common.Extensions;

public static class BinaryReaderExtensions
{
    public static SHA1Hash ReadSHA1Hash(this BinaryReader reader)
    {
        Span<byte> buffer = stackalloc byte[SHA1Hash.HashSizeInBytes];
        int bytesRead = reader.Read(buffer);

        if (bytesRead != SHA1Hash.HashSizeInBytes)
        {
            throw new EndOfStreamException($"Failed to read full SHA1 hash. Expected {SHA1Hash.HashSizeInBytes} bytes, got {bytesRead}.");
        }

        return new SHA1Hash(buffer);
    }

    public static SHA256Hash ReadSHA256Hash(this BinaryReader reader)
    {
        Span<byte> buffer = stackalloc byte[SHA256Hash.HashSizeInBytes];
        int bytesRead = reader.Read(buffer);

        if (bytesRead != SHA256Hash.HashSizeInBytes)
        {
            throw new EndOfStreamException($"Failed to read full SHA256 hash. Expected {SHA256Hash.HashSizeInBytes} bytes, got {bytesRead}.");
        }

        return new SHA256Hash(buffer);
    }
}
