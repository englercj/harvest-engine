// Copyright Chad Engler

namespace Harvest.Kdl;

public interface IKdlObject
{
    void WriteKdl(TextWriter writer, KdlWriteOptions options);
}

public static class KdlObjectExtensions
{
    public static string ToKdlString(this IKdlObject obj, KdlWriteOptions? options = null)
    {
        using StringWriter writer = new();
        obj.WriteKdl(writer, options ?? KdlWriteOptions.PrettyDefault);
        writer.Flush();
        return writer.ToString() ?? string.Empty;
    }
}
