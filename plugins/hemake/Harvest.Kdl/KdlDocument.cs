// Copyright Chad Engler

namespace Harvest.Kdl;

/// <summary>
/// A class representing a KDL document.
/// </summary>
public class KdlDocument : IKdlObject
{
    /// <value>A list of all the <see cref="KdlNode">s found in the document.</value>
    public List<KdlNode> Nodes { get; }

    /// <summary>
    /// Creates a new <see cref="KdlDocument"/> from a file.
    /// </summary>
    /// <param name="filePath">The path to the file to read into a document.</param>
    /// <returns>The parsed document.</returns>
    public static KdlDocument FromFile(string filePath)
    {
        KdlDocumentReadHandler handler = new();
        KdlReader.ReadFile(filePath, handler);
        return handler.Document;
    }

    /// <summary>
    /// Creates a new <see cref="KdlDocument"/> from a string.
    /// </summary>
    /// <param name="filePath">The path name for this file. Only used for source info, the file is not touched and therefore doesn't need to exist on disk.</param>
    /// <param name="str">The string to read data from.</param>
    /// <returns>The parsed document.</returns>
    public static KdlDocument FromString(string filePath, string str)
    {
        KdlDocumentReadHandler handler = new();
        KdlReader.ReadString(filePath, str, handler);
        return handler.Document;
    }

    public KdlDocument()
    {
        Nodes = [];
    }

    public KdlDocument(List<KdlNode> nodes)
    {
        Nodes = nodes;
    }

    public void WriteKdl(TextWriter writer, KdlWriteOptions options)
    {
        if (Nodes.Count == 0)
        {
            writer.Write(options.Newline);
            return;
        }

        foreach (KdlNode node in Nodes)
        {
            node.WriteKdl(writer, options);
            writer.Write(options.Newline);
        }
    }

    public override string ToString() => $"KdlDocument{{ Nodes={{ {string.Join(", ", Nodes)} }} }}";

    public override bool Equals(object? obj)
    {
        if (obj is not KdlDocument other)
        {
            return false;
        }

        return Nodes.SequenceEqual(other.Nodes);
    }

    public override int GetHashCode()
    {
        HashCode hash = new();

        foreach (KdlNode node in Nodes)
        {
            hash.Add(node.GetHashCode());
        }

        return hash.ToHashCode();
    }
}
