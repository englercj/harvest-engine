// Copyright Chad Engler

namespace Harvest.Kdl.Exceptions;

public class KdlException : Exception
{
    private static string AppendContext(string message, KdlSourceInfo? source)
    {
        if (source != null)
        {
            message += $"\nAt line {source.Line}, in position {source.Column}.";
        }

        return message;
    }

    public KdlException(string message, KdlSourceInfo? source)
        : base(AppendContext(message, source))
    { }

    public KdlException(string message, Exception innerException, KdlSourceInfo? source)
        : base(AppendContext(message, source), innerException)
    { }
}
