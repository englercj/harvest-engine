// Copyright Chad Engler

namespace Harvest.Kdl;

/// <summary>
/// A class representing a KDL document.
/// </summary>
public class KdlDocument : IKdlObject
{
    /// <value>A list of all the <see cref="KdlNode">s found in the document.</value>
    public IList<KdlNode> Nodes { get; }

    public static KdlDocument From(Stream stream)
    {
        KdlDocumentReadHandler handler = new();
        KdlReader reader = new();
        reader.Read(stream, handler);
        return handler.Document;
    }

    public static KdlDocument From(string str)
    {
        KdlDocumentReadHandler handler = new();
        KdlReader reader = new();
        reader.Read(str, handler);
        return handler.Document;
    }

    public KdlDocument()
    {
        Nodes = new List<KdlNode>();
    }

    public KdlDocument(IList<KdlNode> nodes)
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
        HashCode hash = new HashCode();

        foreach (KdlNode node in Nodes)
        {
            hash.Add(node.GetHashCode());
        }

        return hash.ToHashCode();
    }
}
