// Copyright Chad Engler

namespace Harvest.Kdl;

public interface IKdlObject
{
    public KdlSourceInfo SourceInfo { get; }

    void WriteKdl(TextWriter writer, KdlWriteOptions options);

    public string ToKdlString(KdlWriteOptions? options = null)
    {
        using StringWriter writer = new();
        WriteKdl(writer, options ?? KdlWriteOptions.PrettyDefault);
        writer.Flush();
        return writer.ToString() ?? "";
    }
}
