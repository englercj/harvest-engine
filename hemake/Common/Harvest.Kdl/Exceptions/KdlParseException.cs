// Copyright Chad Engler

namespace Harvest.Kdl.Exceptions;

public class KdlParseException : Exception
{
    private static string GetMessageString(string message, KdlSourceInfo source)
    {
        return $"KDL parsing failed.\n{source.ToErrorString()}: {message}";
    }

    public KdlParseException(string message, KdlSourceInfo source)
        : base(GetMessageString(message, source))
    { }

    public KdlParseException(string message, Exception innerException, KdlSourceInfo source)
        : base(GetMessageString(message, source), innerException)
    { }
}
