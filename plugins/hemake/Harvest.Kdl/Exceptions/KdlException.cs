// Copyright Chad Engler

namespace Harvest.Kdl.Exceptions;

public class KdlException : Exception
{
    private static string AppendContext(string message, KdlReadContext? context)
    {
        if (context != null)
        {
            message += $"\nAt line {context.Line}, in position {context.Column}.";
        }

        return message;
    }

    public KdlException(string message, KdlReadContext? context)
        : base(AppendContext(message, context))
    { }

    public KdlException(string message, Exception innerException, KdlReadContext? context)
        : base(AppendContext(message, context), innerException)
    { }
}
