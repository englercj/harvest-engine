// Copyright Chad Engler

using Harvest.Kdl.Types;

namespace Harvest.Kdl;

public class KdlNode : IKdlObject
{
    public string Name { get; }
    public string? Type { get; }

    public IList<KdlNode> Children { get; }
    public IList<KdlValue> Arguments { get; }
    public IDictionary<string, KdlValue> Properties { get; }

    public KdlNode(string name, string? type)
    {
        Name = name;
        Type = type;
        Children = new List<KdlNode>();
        Arguments = new List<KdlValue>();
        Properties = new Dictionary<string, KdlValue>();
    }

    public KdlNode(
        string name,
        string? type,
        IList<KdlNode> children,
        IList<KdlValue> arguments,
        IDictionary<string, KdlValue> properties)
    {
        Name = name;
        Type = type;
        Children = children;
        Arguments = arguments;
        Properties = properties;
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

    private void WriteIndent(TextWriter writer, KdlWriteOptions options, int level)
    {
        string indent = new string(options.IndentChar, options.IndentSize);

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
        HashCode hash = new HashCode();

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
