// Copyright Chad Engler

using Harvest.Kdl.Types;

namespace Harvest.Kdl;

public class KdlNode(string name, string? type = null) : IKdlObject
{
    private KdlNode? _parent = null;
    private readonly List<KdlNode> _children = [];

    public string Name => name;
    public string? Type => type;

    public KdlNode? Parent => _parent;
    public IReadOnlyList<KdlNode> Children => _children;

    public List<KdlValue> Arguments { get; } = [];
    public SortedDictionary<string, KdlValue> Properties { get; } = [];

    public KdlSourceInfo SourceInfo { get; set; } = new KdlSourceInfo();

    public void AddChild(KdlNode child)
    {
        child.RemoveFromParent();
        child._parent = this;
        _children.Add(child);
    }

    public void RemoveChild(KdlNode child)
    {
        if (child.Parent == this)
        {
            child._parent = null;
            _children.Remove(child);
        }
    }

    public void RemoveFromParent()
    {
        if (Parent is not null)
        {
            Parent.RemoveChild(this);
        }
    }

    public void ReplaceInParent(KdlNode replacement)
    {
        if (Parent is null)
        {
            return;
        }

        replacement.RemoveFromParent();

        int parentIndex = Parent._children.IndexOf(this);

        Parent._children[parentIndex] = replacement;
        replacement._parent = Parent;

        _parent = null;
    }

    public void ReplaceInParent(IReadOnlyList<KdlNode> replacements)
    {
        if (Parent is null)
        {
            return;
        }

        int parentIndex = Parent._children.IndexOf(this);
        Parent._children.RemoveAt(parentIndex);
        Parent._children.InsertRange(parentIndex, replacements);
        foreach (KdlNode replacement in replacements)
        {
            replacement._parent = Parent;
        }
        _parent = null;
    }

    public void WriteKdl(TextWriter writer, KdlWriteOptions options)
    {
        WriteKdl(writer, options, 0);
    }

    public void WriteKdl(TextWriter writer, KdlWriteOptions options, int level)
    {
        WriteIndent(writer, options, level);

        if (Type != null)
        {
            writer.Write('(');
            KdlUtils.WriteString(writer, Type, options);
            writer.Write(')');
        }

        KdlUtils.WriteString(writer, Name, options);

        foreach (KdlValue argument in Arguments)
        {
            if (argument is KdlNull && !options.PrintNullArguments)
                continue;

            writer.Write(' ');
            argument.WriteKdl(writer, options);
        }

        foreach (KeyValuePair<string, KdlValue> property in Properties)
        {
            if (property.Value is KdlNull && !options.PrintNullProperties)
                continue;

            writer.Write(' ');
            KdlUtils.WriteString(writer, property.Key, options);
            writer.Write('=');
            property.Value.WriteKdl(writer, options);
        }

        if (Children.Count > 0)
        {
            writer.Write(" {");
            writer.Write(options.Newline);

            foreach (KdlNode child in Children)
            {
                child.WriteKdl(writer, options, level + 1);
                writer.Write(options.Newline);
            }

            WriteIndent(writer, options, level);
            writer.Write("}");
        }
        else if (options.PrintEmptyChildren)
        {
            writer.Write(" {}");
        }

        if (options.TerminateNodesWithSemicolon)
        {
            writer.Write(";");
        }
    }

    private static void WriteIndent(TextWriter writer, KdlWriteOptions options, int level)
    {
        string indent = new(options.IndentChar, options.IndentSize);

        for (int i = 0; i < level; ++i)
        {
            writer.Write(indent);
        }
    }

    public override string ToString() => $"KdlNode{{ Name={Name}, Type={Type ?? "null"}, Properties={{ {string.Join(", ", Properties)} }}, Arguments={{ {string.Join(", ", Arguments)} }}, Children={{ {Children} }} }}";

    public override bool Equals(object? obj)
    {
        if (obj is not KdlNode other)
        {
            return false;
        }

        return Name == other.Name
            && Type == other.Type
            && Properties.SequenceEqual(other.Properties)
            && Arguments.SequenceEqual(other.Arguments)
            && Children.SequenceEqual(other.Children);
    }

    public override int GetHashCode()
    {
        HashCode hash = new();

        hash.Add(Name);

        if (Type != null)
        {
            hash.Add(Type);
        }

        foreach (KdlNode child in Children)
        {
            hash.Add(child);
        }

        foreach (KdlValue argument in Arguments)
        {
            hash.Add(argument);
        }

        foreach (KeyValuePair<string, KdlValue> property in Properties)
        {
            hash.Add(property);
        }

        return hash.ToHashCode();
    }

    public KdlValue this[int index]
    {
        get => Arguments[index];
    }

    public KdlValue this[string key]
    {
        get => Properties[key];
    }
}
