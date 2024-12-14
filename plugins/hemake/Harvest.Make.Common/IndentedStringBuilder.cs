// Copyright Chad Engler

using System.Text;

namespace Harvest.Make;

public class IndentedStringBuilder
{
    private readonly StringBuilder _builder = new();
    private int _indentLevel = 0;
    private string _indent = "";

    public IndentedStringBuilder IncreaseIndent()
    {
        ++_indentLevel;
        _indent = new string(' ', _indentLevel * 4);
        return this;
    }

    public IndentedStringBuilder DecreaseIndent()
    {
        if (_indentLevel == 0)
        {
            throw new InvalidOperationException("Indent level cannot be less than 0");
        }

        --_indentLevel;
        _indent = new string(' ', _indentLevel * 4);
        return this;
    }

    public IndentedStringBuilder Append(string value)
    {
        _builder.Append(value);
        return this;
    }

    public IndentedStringBuilder AppendLine()
    {
        _builder.AppendLine();
        return this;
    }

    public IndentedStringBuilder AppendLine(string value)
    {
        _builder.Append(_indent);
        _builder.AppendLine(value);
        return this;
    }

    public IndentedStringBuilder Clear()
    {
        _builder.Clear();
        return this;
    }

    public override string ToString()
    {
        return _builder.ToString();
    }
}
