// Copyright Chad Engler

namespace Harvest.Kdl;

/// <summary>
/// A class representing a KDL document.
/// </summary>
public class KdlDocument : IKdlObject
{
    private readonly KdlNode _root = new("<root>");

    /// <value>A list of all the <see cref="KdlNode">s in the document.</value>
    public IReadOnlyList<KdlNode> Nodes => _root.Children;

    /// <value>The source info of the root of the document</value>
    public KdlSourceInfo SourceInfo => _root.SourceInfo;

    /// <summary>
    /// Creates a new <see cref="KdlDocument"/> from a file.
    /// </summary>
    /// <param name="filePath">The path to the file to read into a document.</param>
    /// <returns>The parsed document.</returns>
    public static KdlDocument FromFile(string filePath)
    {
        KdlDocumentReadHandler handler = new();
        KdlReader.ReadFile(filePath, handler);
        handler.Document._root.SourceInfo = new KdlSourceInfo(filePath, 1, 1);
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
        handler.Document._root.SourceInfo = new KdlSourceInfo(filePath, 1, 1);
        return handler.Document;
    }

    public IEnumerable<KdlNode> GetNodesByName(string name)
    {
        return GetChildNodesByName(name, _root);
    }

    public void AddChild(KdlNode node)
    {
        _root.AddChild(node);
    }

    private static IEnumerable<KdlNode> GetChildNodesByName(string name, KdlNode scope)
    {
        foreach (KdlNode child in scope.Children)
        {
            if (child.Name == name)
            {
                yield return child;
            }

            foreach (KdlNode found in GetChildNodesByName(name, child))
            {
                yield return found;
            }
        }
    }

    public void WriteKdl(TextWriter writer, KdlWriteOptions options)
    {
        if (Nodes.Count == 0)
        {
            writer.Write(options.Newline);
            return;
        }

        foreach (KdlNode child in Nodes)
        {
            child.WriteKdl(writer, options);
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
        return _root.GetHashCode();
    }
}
