// Copyright Chad Engler

using Harvest.Kdl;

namespace Harvest.Make.Projects;

public class NodeParseException : Exception
{
    public KdlNode Node { get; }

    public NodeParseException(KdlNode node, string message)
        : base(BuildMessage(node, message))
    {
        Node = node;
    }

    public NodeParseException(KdlNode node, string message, Exception? innerException)
        : base(BuildMessage(node, message), innerException)
    {
        Node = node;
    }

    private static string BuildMessage(KdlNode node, string message)
    {
        return $"Failed to parse '{node.Name}' node.\n{node.SourceInfo.ToErrorString()}: {message}";
    }
}
