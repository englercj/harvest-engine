namespace Harvest.Common.Extensions;

public static class BinaryWriterExtensions
{
    public static void Write(this BinaryWriter writer, SHA1Hash hash)
    {
        writer.Write(hash.Data);
    }

    public static void Write(this BinaryWriter writer, SHA256Hash hash)
    {
        writer.Write(hash.Data);
    }
}
