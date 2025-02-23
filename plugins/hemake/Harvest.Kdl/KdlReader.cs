// Copyright Chad Engler

using Harvest.Kdl.Exceptions;
using Harvest.Kdl.Types;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;

namespace Harvest.Kdl;

public class KdlReader(string filePath, byte[] data, IKdlReadHandler handler, KdlReadOptions? options) : object()
{
    private const int InvalidCodePoint = -1;
    private const int MaxNodeDepth = 8192;

    private readonly string _filePath = filePath;
    private readonly byte[] _bytes = data;
    private readonly KdlReadOptions _options = options ?? new KdlReadOptions();
    private readonly IKdlReadHandler _handler = handler;

    private int _line = 1;
    private int _offset = 0;
    private int _lineStartOffset = 0;

    private bool _hasPendingType = false;
    private bool _inWhitespaceEscape = false;
    private int _nodeDepth = 0;

    private readonly List<byte> _stringBuffer = new(256);
    private readonly List<int> _slashDashDepthStack = [];

    public static void ReadFile(string filePath, IKdlReadHandler handler, KdlReadOptions? options = null)
    {
        byte[] data = File.ReadAllBytes(filePath);
        KdlReader kdlReader = new(filePath, data, handler, options);
        kdlReader.Parse();
    }

    public static void ReadString(string filePath, string str, IKdlReadHandler handler, KdlReadOptions? options = null)
    {
        byte[] data = Encoding.UTF8.GetBytes(str);
        KdlReader kdlReader = new(filePath, data, handler, options);
        kdlReader.Parse();
    }

    protected void Parse()
    {
        ConsumeBOM();

        _handler.StartDocument(GetSourceInfo());

        ConsumeVersion();

        while (!AtEnd())
        {
            ParseExpression();
        }

        ThrowIf(IsAnySlashdashOpen(), "Invalid token. Trailing slashdash at the end of the document.");

        _handler.EndDocument();
    }

    protected void ThrowIf(bool condition, string message)
    {
        if (condition)
        {
            throw new KdlException(message, GetSourceInfo());
        }
    }

    protected void ThrowIfAtEnd()
    {
        ThrowIf(AtEnd(), "Unexpected end of file.");
    }

    protected string GetBufferedString()
    {
        return Encoding.UTF8.GetString(CollectionsMarshal.AsSpan(_stringBuffer));
    }

    protected KdlSourceInfo GetSourceInfo()
    {
        int column = Encoding.UTF8.GetCharCount(_bytes.AsSpan(_lineStartOffset, _offset - _lineStartOffset)) + 1;
        return new KdlSourceInfo(_filePath, _line, column);
    }

    protected int PeekCodePoint(byte[] bytes, int offset, out int len)
    {
        ThrowIf(offset >= bytes.Length, "Unexpected end of file.");

        len = UTF8Decode(out int ucc, bytes, offset);

        ThrowIf(len == 0, "Unexpected end of file.");
        ThrowIf(len == InvalidCodePoint, "Invalid UTF-8 sequence.");
        ThrowIf(KdlUtils.IsDisallowed(ucc), "Disallowed character found.");

        return ucc;
    }

    protected int PeekCodePoint(out int len)
    {
        return PeekCodePoint(_bytes, _offset, out len);
    }

    protected int ConsumeCodePoint(byte[] bytes, ref int offset)
    {
        int ucc = PeekCodePoint(bytes, offset, out int len);

        offset += len;
        return ucc;
    }

    protected int ConsumeCodePoint()
    {
        return ConsumeCodePoint(_bytes, ref _offset);
    }

    protected bool AtEnd()
    {
        return _offset >= _bytes.Length;
    }

    protected void SkipSpaces(bool allowSlashdash = true)
    {
        while (!AtEnd())
        {
            int ucc = PeekCodePoint(out int len);

            // catch escape sequences for escaped newlines
            if (ucc == '\\')
            {
                _inWhitespaceEscape = true;
                _offset += len;
            }
            // anywhere you can have whitespace, you can also have comments
            else if (ucc == '/')
            {
                ucc = PeekCodePoint(_bytes, _offset + len, out _);

                // Single line comments and slashdash comments are terminators for nodes, they don't count just "spaces"
                if (ucc == '/')
                {
                    return;
                }

                // Some contexts don't allow slashdash comments
                if (!allowSlashdash && ucc == '-')
                {
                    return;
                }

                ParseComment();
            }
            // if it is whitespace consume it and continue
            else if (KdlUtils.IsWhitespace(ucc))
            {
                _offset += len;
            }
            // escaped newlines are considered normal spaces
            else if (KdlUtils.IsNewline(ucc) && _inWhitespaceEscape)
            {
                ConsumeNewLine();
            }
            // finally, if it isn't whitespace or comment we're done here
            else
            {
                _inWhitespaceEscape = false;
                break;
            }
        }
    }

    protected void ConsumeBOM()
    {
        if (_offset < _bytes.Length && _bytes[_offset] != 0xef)
        {
            return;
        }

        ++_offset;
        ThrowIf(_offset >= _bytes.Length || _bytes[_offset] != 0xbb, "Invalid BOM");

        ++_offset;
        ThrowIf(_offset >= _bytes.Length || _bytes[_offset] != 0xbf, "Invalid BOM");

        ++_offset;
    }

    protected void ConsumeVersion()
    {
        int begin = _offset;

        if (begin >= _bytes.Length || _bytes[begin] != '/'
            || begin + 1 >= _bytes.Length || _bytes[begin + 1] != '-')
        {
            // Not a slashdash, so not a version marker
            return;
        }

        begin += 2;

        // Skip past any unicode spaces before the marker
        while (begin < _bytes.Length)
        {
            int ucc = PeekCodePoint(_bytes, begin, out int len);

            if (!KdlUtils.IsWhitespace(ucc))
            {
                break;
            }

            begin += len;
        }

        byte[] versionMarker = Encoding.UTF8.GetBytes("kdl-version");
        if (begin + versionMarker.Length < _bytes.Length
            && _bytes.AsSpan(begin, versionMarker.Length).SequenceEqual(versionMarker))
        {
            begin += versionMarker.Length;
        }
        else
        {
            // This is a slashdash comment, not a version marker
            return;
        }

        // Skip past any unicode spaces after the marker, there must be at least one
        bool hasWhitespace = false;
        while (begin < _bytes.Length)
        {
            int ucc = PeekCodePoint(_bytes, begin, out int len);

            if (!KdlUtils.IsWhitespace(ucc))
            {
                break;
            }

            hasWhitespace = true;
            begin += len;
        }

        // This is a slashdash comment, not a version marker
        if (!hasWhitespace || begin >= _bytes.Length)
        {
            return;
        }

        // Only version 2 is supported
        ThrowIf(_bytes[begin] != '2', "Invalid Version. Only KDL v2 is supported.");

        string version = char.ToString((char)_bytes[begin]);
        ++begin;

        ThrowIf(begin >= _bytes.Length, "Unexpected end of file.");

        while (begin < _bytes.Length)
        {
            int ucc = PeekCodePoint(_bytes, begin, out int len);

            if (KdlUtils.IsWhitespace(ucc))
            {
                begin += len;
                continue;
            }

            if (KdlUtils.IsNewline(ucc))
            {
                begin += len;
                break;
            }

            throw new KdlException("Invalid token. Expected newline after version marker.", GetSourceInfo());
        }

        _offset = begin;
        _handler.Version(version, GetSourceInfo());
    }

    protected void Consume(char ch)
    {
        ThrowIfAtEnd();

        int ucc = PeekCodePoint(out int len);

        ThrowIf(ucc != ch, $"Invalid token. Expected '{ch}' but found '{char.ConvertFromUtf32(ucc)}'.");

        _offset += len;
    }

    protected void Consume(string str)
    {
        foreach (char ch in str)
        {
            Consume(ch);
        }
    }

    protected void ConsumeNewLine()
    {
        ThrowIfAtEnd();

        int ucc = PeekCodePoint(out int len);

        ThrowIf(!KdlUtils.IsNewline(ucc), $"Invalid token. Expected newline but found '{char.ConvertFromUtf32(ucc)}'.");

        _offset += len;

        if (ucc == '\r' && !AtEnd())
        {
            ucc = PeekCodePoint(out len);

            if (ucc == '\n')
            {
                _offset += len;
            }
        }

        ++_line;
        _lineStartOffset = _offset;
        _inWhitespaceEscape = false;
    }

    protected bool IsEndOfString(int begin, int rawDelimCount)
    {
        int count = 0;
        while (begin < _bytes.Length && _bytes[begin] != '\0' && count < rawDelimCount)
        {
            int ucc = ConsumeCodePoint(_bytes, ref begin);

            if (ucc != '#')
            {
                break;
            }

            ++count;
        }

        return count == rawDelimCount;
    }

    protected bool IsAnySlashdashOpen()
    {
        return _slashDashDepthStack.Count != 0;
    }

    protected bool IsSlashdashOpen()
    {
        return _slashDashDepthStack.Count != 0 && _slashDashDepthStack[^1] == _nodeDepth;
    }

    protected void PushOpenSlashdash()
    {
        // Slashdash after a type is not allowed
        ThrowIf(_hasPendingType, "Invalid Token. Slashdash is not allowed after a type.");

        _slashDashDepthStack.Add(_nodeDepth);
        _handler.StartComment(GetSourceInfo());
    }

    protected void PopOpenSlashdash()
    {
        if (_slashDashDepthStack.Count > 0 && _slashDashDepthStack[^1] == _nodeDepth)
        {
            _slashDashDepthStack.RemoveAt(_slashDashDepthStack.Count - 1);
            _handler.EndComment();
        }
    }

    protected byte[] GetDedentPrefix(int rawDelimCount)
    {
        byte[] dedentPrefix = [];

        int begin = _offset;
        int lineStart = begin;
        bool inEscapeSequence = false;
        while (begin < _bytes.Length && _bytes[begin] != '\0')
        {
            int ucc = ConsumeCodePoint(_bytes, ref begin);

            // Hitting any non-whitespace character while in an escape sequence will end it.
            // We're not parsing escape sequences, just searching for the end of the string.
            // The only reason we check for escaped whitespace is to ensure that `lineStart`
            // only changes when hitting an unescaped newline.
            if (inEscapeSequence)
            {
                if (!KdlUtils.IsWhitespace(ucc) && !KdlUtils.IsNewline(ucc))
                {
                    inEscapeSequence = false;
                }
            }
            // If we're not in an escape sequence and we hit a backslash,
            // this is the start of a new escape sequence.
            else if (ucc == '\\')
            {
                inEscapeSequence = true;
            }

            // Check for send of string and store the dedent prefix
            if (!inEscapeSequence)
            {
                if (ucc == '"'
                    && begin + 0 < _bytes.Length && _bytes[begin + 0] == '"'
                    && begin + 1 < _bytes.Length && _bytes[begin + 1] == '"')
                {
                    if (IsEndOfString(begin + 2, rawDelimCount))
                    {
                        // -1 for the consumed quote
                        dedentPrefix = _bytes.AsSpan(lineStart, begin - lineStart - 1).ToArray();
                        break;
                    }
                }

                if (KdlUtils.IsNewline(ucc))
                {
                    lineStart = begin;
                }
            }
        }

        // This shouldn't happen, but if it does, it's an error.
        ThrowIf(inEscapeSequence, "Invalid escape sequence.");

        // Validate that the final line of the string is only whitespace, or escaped whitespace
        int dedentOffset = 0;
        while (dedentOffset < dedentPrefix.Length)
        {
            int ucc = ConsumeCodePoint(dedentPrefix, ref dedentOffset);

            if (!inEscapeSequence && ucc == '\\')
            {
                inEscapeSequence = true;
                continue;
            }

            bool allowed = KdlUtils.IsWhitespace(ucc) || (inEscapeSequence && KdlUtils.IsNewline(ucc));
            ThrowIf(!allowed, "Invalid Token. Expected whitespace character.");
        }

        return dedentPrefix;
    }

    protected void ConsumeDedentPrefix(byte[] dedentPrefix)
    {
        // Empty lines without a dedent prefix are allowed.
        int ucc = PeekCodePoint(out int len);

        if (KdlUtils.IsNewline(ucc))
        {
            return;
        }

        int dedentOffset = 0;
        while (dedentOffset < dedentPrefix.Length)
        {
            int prefixUcc = ConsumeCodePoint(dedentPrefix, ref dedentOffset);

            // We know at this point that the dedent prefix is only whitespace characters.
            // So we can assume if we see an backslash, it's a whitespace escape sequence
            // and we can skip the rest of the prefix.
            if (prefixUcc == '\\')
            {
                break;
            }

            ThrowIf(ucc != prefixUcc, "Invalid Token. Unmatched prefix in multiline string.");

            _offset += len;
            ucc = PeekCodePoint(out len);
        }
    }

    protected string ConsumeType()
    {
        Consume('(');
        SkipSpaces();
        string type = ConsumeString();
        SkipSpaces();
        Consume(')');
        SkipSpaces();

        _hasPendingType = true;
        return type;
    }

    protected int ConsumeUnicodeEscapeSequence()
    {
        Consume('u');
        Consume('{');
        ThrowIfAtEnd();

        // Max of 6 hex characters
        bool foundNum = false;
        int ucc = 0;
        for (int i = 0; i < 6; ++i)
        {
            ThrowIfAtEnd();

            byte ch = _bytes[_offset];
            if (ch == '}')
            {
                break;
            }

            ThrowIf(!KdlUtils.IsHexadecimalDigit(ch), $"Invalid token. Expected hex digit but found '{(char)ch}'.");

            ucc = (ucc << 4) + KdlUtils.HexToNibble((char)ch);
            foundNum = true;
            ++_offset;
        }

        Consume('}');

        ThrowIf(!foundNum, "Invalid unicode escape sequence. No hex digits found in braces.");
        ThrowIf(!KdlUtils.IsUnicodeScalarValue(ucc), "Invalid unicode escape sequence. Value is not a valid unicode scalar value.");

        return ucc;
    }

    protected string ConsumeQuotedString(int rawDelimCount)
    {
        _stringBuffer.Clear();

        bool firstChar = true;
        bool isMultiline = false;
        bool inEscapeSeq = false;
        byte[] dedentPrefix = [];
        while (!AtEnd())
        {
            int ucc = PeekCodePoint(out int len);

            // Multi-line string handling
            if (firstChar)
            {
                firstChar = false;

                if (ucc == '"')
                {
                    if (_offset + 1 < _bytes.Length && _bytes[_offset + 1] == '"')
                    {
                        isMultiline = true;
                    }
                }

                if (isMultiline)
                {
                    _offset += 2;

                    ConsumeNewLine();

                    // Multi-line strings dedent by the number of spaces on the last line
                    // of the string. This means we need to scan to the end of the string
                    // and store the whitespace prefix of the final line.
                    dedentPrefix = GetDedentPrefix(rawDelimCount);

                    // TODO: Only consume the dedent prefix if the line has non-whitespace characters
                    // See: https://github.com/kdl-org/kdl/issues/503
                    ConsumeDedentPrefix(dedentPrefix);

                    continue;
                }
            }

            // Escape sequence handling
            if (inEscapeSeq)
            {
                if (KdlUtils.IsWhitespace(ucc) || KdlUtils.IsNewline(ucc))
                {
                    do
                    {
                        _offset += len;
                        ucc = PeekCodePoint(out len);
                    } while (KdlUtils.IsWhitespace(ucc) || KdlUtils.IsNewline(ucc));

                    inEscapeSeq = false;
                    continue;
                }

                if (ucc == 'u')
                {
                    UTF8Encode(_stringBuffer, ConsumeUnicodeEscapeSequence());
                    inEscapeSeq = false;
                    continue;
                }

                switch (ucc)
                {
                    case 'n': _stringBuffer.Add((byte)'\n'); break;
                    case 'r': _stringBuffer.Add((byte)'\r'); break;
                    case 't': _stringBuffer.Add((byte)'\t'); break;
                    case '\\': _stringBuffer.Add((byte)'\\'); break;
                    case '"': _stringBuffer.Add((byte)'"'); break;
                    case 'b': _stringBuffer.Add((byte)'\b'); break;
                    case 'f': _stringBuffer.Add((byte)'\f'); break;
                    case 's': _stringBuffer.Add((byte)' '); break;
                    default:
                        throw new KdlException($"Invalid escape sequence '\\{char.ConvertFromUtf32(ucc)}'.", GetSourceInfo());
                }

                inEscapeSeq = false;
                _offset += len;
                continue;
            }

            // Newline handling is different for multi-line or single-line strings
            if (KdlUtils.IsNewline(ucc))
            {
                ThrowIf(!isMultiline, "Invalid Token. Unescaped newlines not allowed in single-line strings.");

                ConsumeNewLine();

                // Consume the required dedent prefix since we consumed a newline
                ConsumeDedentPrefix(dedentPrefix);

                // newlines are normalized to `\n`
                _stringBuffer.Add((byte)'\n');
                continue;
            }

            // New escape sequence start handling
            if (ucc == '\\')
            {
                if (rawDelimCount > 0)
                {
                    _stringBuffer.Add((byte)'\\');
                }
                else
                {
                    inEscapeSeq = true;
                }

                _offset += len;
                continue;
            }

            // End of string handling
            if (ucc == '"')
            {
                bool isMultilineEnd = isMultiline
                    && _offset + 1 < _bytes.Length && _bytes[_offset + 1] == '"'
                    && _offset + 2 < _bytes.Length && _bytes[_offset + 2] == '"';

                if (!isMultiline || isMultilineEnd)
                {
                    int multilineOffset = isMultilineEnd ? 2 : 0;
                    int strEnd = _offset + len + multilineOffset;
                    if (IsEndOfString(strEnd, rawDelimCount))
                    {
                        _offset = strEnd;
                        _offset += rawDelimCount;
                        if (_stringBuffer.Count != 0 && isMultiline && _stringBuffer[^1] == (byte)'\n')
                        {
                            _stringBuffer.RemoveAt(_stringBuffer.Count - 1);
                        }
                        return GetBufferedString();
                    }
                }
            }

            // Append the character to the buffer
            UTF8Encode(_stringBuffer, ucc);
            _offset += len;
        }

        throw new KdlException("Unexpected end of file.", GetSourceInfo());
    }

    protected string ConsumeIdentifierString()
    {
        int begin = _offset;

        bool first = true;
        while (!AtEnd())
        {
            int ucc = PeekCodePoint(out int len);

            ThrowIf(first && !KdlUtils.IsValidInitialIdentifierCharacter(ucc),
                $"Invalid token. Expected initial identifier character but found '{char.ConvertFromUtf32(ucc)}'.");

            first = false;

            if (!KdlUtils.IsValidIdentifierCharacter(ucc))
            {
                break;
            }

            _offset += len;
        }

        ReadOnlySpan<byte> value = _bytes.AsSpan(begin, _offset - begin);

        // No identifier string can start with a number
        ThrowIf(KdlUtils.IsDecimalDigit(value[0]), "Invalid identifier. Identifier strings cannot start with a number.");

        if (value[0] == '-' || value[0] == '+')
        {
            // Identifiers can start with "-."/"+." as long as the next char is not a number
            ThrowIf(value.Length > 2 && value[1] == '.' && KdlUtils.IsDecimalDigit(value[2]),
                "Invalid identifier. Identifier strings cannot start with a dash ('-') or plus ('+'), followed by a dot ('.') and a number.");

            // Identifiers can start with "-"/"+" as long as the next char is not a number
            ThrowIf(value.Length > 1 && KdlUtils.IsDecimalDigit(value[1]),
                "Invalid identifier. Identifier strings cannot start with a dash ('-') or plus ('+'), followed by a number.");
        }

        if (value[0] == '.')
        {
            // Identifiers can start with "." as long as the next char is not a number
            ThrowIf(value.Length > 1 && KdlUtils.IsDecimalDigit(value[1]),
                "Invalid identifier. Identifier strings cannot start with a dot ('.'), followed by a number.");
        }

        string str = Encoding.UTF8.GetString(value);

        // Identifiers can't be any of the language keywords without their leading '#'
        ThrowIf(str == "inf" || str == "-inf" || str == "nan" || str == "true" || str == "false" || str == "null",
            "Invalid identifier. Identifier strings cannot be a language keyword.");

        return str;
    }

    protected string ConsumeString()
    {
        ThrowIfAtEnd();

        int ucc = PeekCodePoint(out int len);

        if (ucc == '"')
        {
            _offset += len;
            return ConsumeQuotedString(0);
        }

        if (ucc == '#')
        {
            _offset += len;
            int rawDelimCount = 1;
            while (!AtEnd())
            {
                ucc = ConsumeCodePoint();

                if (ucc == '#')
                {
                    ++rawDelimCount;
                    continue;
                }

                if (ucc == '"')
                {
                    return ConsumeQuotedString(rawDelimCount);
                }
            }

            ThrowIfAtEnd();
        }

        return ConsumeIdentifierString();
    }

    protected void ParseExpression()
    {
        SkipSpaces();

        if (AtEnd())
        {
            if (_nodeDepth > 0)
            {
                ThrowIfAtEnd();
            }
            return;
        }

        int ucc = PeekCodePoint(out int len);

        if (KdlUtils.IsNewline(ucc))
        {
            ConsumeNewLine();
            return;
        }

        switch (ucc)
        {
            case '/':
            {
                ParseComment();
                break;
            }
            case ';':
            {
                // empty node terminated by semicolon
                _offset += len;
                break;
            }
            case '(':
            case '"':
            case '#':
            {
                // start of type annotation or string name of node
                ParseNode();
                break;
            }
            default:
            {
                if (KdlUtils.IsValidInitialIdentifierCharacter(ucc))
                {
                    ParseNode();
                }
                else
                {
                    throw new KdlException($"Invalid token. Expected node start but found '{char.ConvertFromUtf32(ucc)}'.", GetSourceInfo());
                }
                break;
            }
        }
    }

    protected void ParseComment()
    {
        KdlSourceInfo source = GetSourceInfo();
        Consume('/');
        ThrowIfAtEnd();

        int ucc = PeekCodePoint(out int len);

        switch (ucc)
        {
            case '-':
            {
                // slashdash
                _offset += len;
                PushOpenSlashdash();
                return;
            }
            case '/':
            {
                // single-line comment
                _offset += len;
                SkipSpaces();

                int begin = _offset;

                // Scan forward until we find a newline
                while (!AtEnd())
                {
                    ucc = PeekCodePoint(out len);

                    if (KdlUtils.IsNewline(ucc))
                    {
                        break;
                    }

                    _offset += len;
                }

                ReadOnlySpan<byte> value = _bytes.AsSpan(begin, _offset - begin);
                string comment = Encoding.UTF8.GetString(value);
                _handler.Comment(comment.Trim(), source);

                if (!AtEnd())
                {
                    ConsumeNewLine();
                }
                break;
            }
            case '*':
            {
                // multi-line comment
                _offset += len;
                SkipSpaces();

                _stringBuffer.Clear();

                int prevUcc = 0;
                int depth = 1;
                while (depth > 0)
                {
                    ThrowIfAtEnd();

                    ucc = PeekCodePoint(out len);

                    if (ucc == '*' && prevUcc == '/')
                    {
                        ++depth;
                        ucc = 0; // reset so "/*/" isn't self-closing
                    }
                    else if (ucc == '/' && prevUcc == '*')
                    {
                        --depth;
                        ucc = 0; // reset so "*/*" isn't self-opening
                    }

                    if (KdlUtils.IsNewline(ucc))
                    {
                        ConsumeNewLine();

                        // newlines are normalized to '\n'
                        _stringBuffer.Add((byte)'\n');
                    }
                    else
                    {
                        UTF8Encode(_stringBuffer, ucc);
                        _offset += len;
                    }

                    prevUcc = ucc;
                }

                // remove the trailing "*/"
                if (_stringBuffer.Count > 2)
                {
                    _stringBuffer.RemoveAt(_stringBuffer.Count - 1);
                    _stringBuffer.RemoveAt(_stringBuffer.Count - 1);
                }

                string comment = GetBufferedString();
                _handler.Comment(comment.Trim(), source);
                break;
            }
            default:
            {
                throw new KdlException($"Invalid token. Expected '-', '/', or '*' but found '{char.ConvertFromUtf32(ucc)}'.", GetSourceInfo());
            }
        }
    }

    protected void ParseNode()
    {
        SkipSpaces();
        KdlSourceInfo source = GetSourceInfo();

        int ucc = PeekCodePoint(out _);

        string? type = null;
        if (ucc == '(')
        {
            type = ConsumeType();
        }

        string name = ConsumeString();

        EmitStartNode(name, type, source);

        if (AtEnd())
        {
            EmitEndNode();
            return;
        }

        ucc = PeekCodePoint(out _);

        ThrowIf(!KdlUtils.IsWhitespace(ucc) && !KdlUtils.IsNewline(ucc) && ucc != ';' && ucc != '/' && ucc != '{' && ucc != '}',
            $"Invalid token. Expected whitespace, newline, semicolon, comment, or end of node but found '{char.ConvertFromUtf32(ucc)}'.");

        bool hasWhitespace = false;
        bool hasAnyPropOrArg = false;
        bool hasAnyChildBlock = false;
        bool hasActiveChildBlock = false;
        while (!AtEnd())
        {
            ucc = PeekCodePoint(out int len);

            // Newline is a node terminator if no slashdash is active.
            if (KdlUtils.IsNewline(ucc))
            {
                ConsumeNewLine();

                if (!IsSlashdashOpen())
                {
                    EmitEndNode();
                    return;
                }

                continue;
            }

            // Whitespace is required between the node name and the first argument or property
            if (KdlUtils.IsWhitespace(ucc))
            {
                hasWhitespace = true;
                SkipSpaces();
                continue;
            }

            // consume an argument, property, or children
            switch (ucc)
            {
                // Single-line comment is a node terminator, end the node and then consume it.
                // Multi-line and slashdash comments are allowed in the node
                case '/':
                {
                    if (!IsSlashdashOpen() && _offset + 1 < _bytes.Length && _bytes[_offset + 1] == '/' && !_inWhitespaceEscape)
                    {
                        EmitEndNode();
                        ParseComment();
                        return;
                    }

                    ParseComment();
                    break;
                }
                // Semicolon is a node terminator, end the node and then consume it.
                case ';':
                {
                    EmitEndNode();
                    _offset += len;
                    return;
                }
                // End brace is a node terminator, end the node but don't consume it.
                // Let ParseNodeChildBlock consume it when handling the child block.
                case '}':
                {
                    ThrowIf(_nodeDepth == 0, "Invalid Token. Encountered an unmatched end brace.");
                    EmitEndNode();
                    return;
                }
                // Open brace indicates we're starting a child block.
                case '{':
                {
                    ThrowIf(hasAnyPropOrArg && !hasWhitespace, "Invalid token. Expected whitespace before child block.");

                    if (hasActiveChildBlock)
                    {
                        ThrowIf(!IsSlashdashOpen(), "Invalid token. Nodes cannot have two child blocks.");
                    }
                    else
                    {
                        hasActiveChildBlock = !IsSlashdashOpen();
                    }

                    hasAnyChildBlock = true;
                    _offset += len;

                    ParseNodeChildBlock();
                    break;
                }
                // either a keyword value, or start of a raw string
                case '#':
                // a type annotation here can only be for an argument value
                case '(':
                // a number here can only be an argument value
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                // start of a string could be an argument value or property name
                case '"':
                {
                    ThrowIf(!IsSlashdashOpen() && !hasWhitespace, "Invalid token. Expected whitespace before a property or argument.");
                    ThrowIf(hasAnyChildBlock, "Invalid token. Arguments and properties must come before child blocks.");

                    ParseArgumentOrProperty();
                    hasWhitespace = false;
                    break;
                }
                // anything else must be an identifier string
                // which could be an argument value or property name
                default:
                {
                    ThrowIf(!IsSlashdashOpen() && !hasWhitespace, "Invalid token. Expected whitespace before a property or argument.");
                    ThrowIf(!KdlUtils.IsValidInitialIdentifierCharacter(ucc), "Invalid token. Expected an initial identifier character.");
                    ThrowIf(hasAnyChildBlock, "Invalid token. Arguments and properties must come before child blocks.");

                    ParseArgumentOrProperty();
                    hasAnyPropOrArg = true;
                    hasWhitespace = false;
                    break;
                }
            }
        }

        // End the node if we reach EOF
        EmitEndNode();
    }

    void ParseNodeChildBlock()
    {
        // Extra depth for child blocks ensures that we don't confuse a slashdash
        // on a child node with the slashdash on the child node block.
        // This will be decremented when the child block ends.
        ++_nodeDepth;

        int depth = _nodeDepth;
        while (_nodeDepth >= depth)
        {
            SkipSpaces();
            ThrowIfAtEnd();

            int ucc = PeekCodePoint(out int len);

            // End the child block
            if (ucc == '}')
            {
                _offset += len;
                break;
            }

            ParseExpression();
        }

        ThrowIf(_nodeDepth == 0, "Invalid Token. Unexpected end of parent node.");
        ThrowIf(IsSlashdashOpen(), "Invalid Token. Trailing slashdash at the end of child block.");

        // End the child block's extra depth
        --_nodeDepth;

        // Complete any slashdash for the child block
        PopOpenSlashdash();
    }

    protected void EmitStartNode(string name, string? type, KdlSourceInfo source)
    {
        _handler.StartNode(name, type, source);
        ++_nodeDepth;

        ThrowIf(_nodeDepth > MaxNodeDepth, "Max Node Depth Exceeded.");
        _hasPendingType = false;
    }

    protected void EmitEndNode()
    {
        ThrowIf(_nodeDepth == 0, "Invalid Token. Unexpected end of node.");
        ThrowIf(IsSlashdashOpen(), "Invalid Token. Trailing slashdash at the end of node.");

        _handler.EndNode();
        --_nodeDepth;
        PopOpenSlashdash();
    }

    protected void EmitPropOrArg(KdlValue kdlValue, string? name, KdlSourceInfo source)
    {
        if (name != null)
        {
            _handler.Property(name, kdlValue, source);
        }
        else
        {
            _handler.Argument(kdlValue, source);
        }

        PopOpenSlashdash();
        _hasPendingType = false;
    }

    protected void EmitPropOrArg(object? value, string? type, string? name, KdlSourceInfo source)
    {
        KdlValue kdlValue = KdlValue.From(value, type);
        EmitPropOrArg(kdlValue, name, source);
    }

    protected bool ParseNumWithType<T>(KdlNumber<T> num, int sign, string type, string? propName, KdlSourceInfo source) where T : struct, INumber<T>
    {
        switch (type)
        {
            case "u8":
            case "u16":
            case "u32":
            case "u64":
            case "usize":
                ThrowIf(sign != 1, "Invalid number. Unsigned types cannot be negative.");
                break;
        }

        switch (type)
        {
            case "u8": EmitPropOrArg(num.Value is byte ? num : new KdlNumber<byte>((byte)Convert.ChangeType(num.Value, typeof(byte)), num.Radix, num.Type), propName, source); return true;
            case "u16": EmitPropOrArg(num.Value is ushort ? num : new KdlNumber<ushort>((ushort)Convert.ChangeType(num.Value, typeof(ushort)), num.Radix, num.Type), propName, source); return true;
            case "u32": EmitPropOrArg(num.Value is uint ? num : new KdlNumber<uint>((uint)Convert.ChangeType(num.Value, typeof(uint)), num.Radix, num.Type), propName, source); return true;
            case "u64": EmitPropOrArg(num.Value is ulong ? num : new KdlNumber<ulong>((ulong)Convert.ChangeType(num.Value, typeof(ulong)), num.Radix, num.Type), propName, source); return true;
            case "i8": EmitPropOrArg(num.Value is sbyte ? num : new KdlNumber<sbyte>((sbyte)Convert.ChangeType(num.Value, typeof(sbyte)), num.Radix, num.Type), propName, source); return true;
            case "i16": EmitPropOrArg(num.Value is short ? num : new KdlNumber<short>((short)Convert.ChangeType(num.Value, typeof(short)), num.Radix, num.Type), propName, source); return true;
            case "i32": EmitPropOrArg(num.Value is int ? num : new KdlNumber<int>((int)Convert.ChangeType(num.Value, typeof(int)), num.Radix, num.Type), propName, source); return true;
            case "i64": EmitPropOrArg(num.Value is long ? num : new KdlNumber<long>((long)Convert.ChangeType(num.Value, typeof(long)), num.Radix, num.Type), propName, source); return true;
            case "isize": EmitPropOrArg(num.Value is nint ? num : new KdlNumber<nint>((nint)Convert.ChangeType(num.Value, typeof(nint)), num.Radix, num.Type), propName, source); return true;
            case "usize": EmitPropOrArg(num.Value is nuint ? num : new KdlNumber<nuint>((nuint)Convert.ChangeType(num.Value, typeof(nuint)), num.Radix, num.Type), propName, source); return true;
            case "f32": EmitPropOrArg(num.Value is float ? num : new KdlNumber<float>((float)Convert.ChangeType(num.Value, typeof(float)), num.Radix, num.Type), propName, source); return true;
            case "f64": EmitPropOrArg(num.Value is double ? num : new KdlNumber<double>((double)Convert.ChangeType(num.Value, typeof(double)), num.Radix, num.Type), propName, source); return true;
            case "decimal64": EmitPropOrArg(num.Value is double ? num : new KdlNumber<double>((double)Convert.ChangeType(num.Value, typeof(double)), num.Radix, num.Type), propName, source); return true;
            case "decimal": EmitPropOrArg(num.Value is decimal ? num : new KdlNumber<decimal>((decimal)Convert.ChangeType(num.Value, typeof(decimal)), num.Radix, num.Type), propName, source); return true;
            case "decimal128": EmitPropOrArg(num.Value is decimal ? num : new KdlNumber<decimal>((decimal)Convert.ChangeType(num.Value, typeof(decimal)), num.Radix, num.Type), propName, source); return true;
        }

        return false;
    }

    protected bool ParseNumWithType(int radix, int sign, string type, string? propName, KdlSourceInfo source)
    {
        switch (type)
        {
            case "u8":
            case "u16":
            case "u32":
            case "u64":
            case "usize":
                ThrowIf(sign == -1, "Invalid number. Unsigned types cannot be negative.");
                break;
        }

        switch (type)
        {
            case "u8": EmitPropOrArg(KdlUtils.ParseUInt8(GetBufferedString(), radix, type), propName, source); return true;
            case "u16": EmitPropOrArg(KdlUtils.ParseUInt16(GetBufferedString(), radix, type), propName, source); return true;
            case "u32": EmitPropOrArg(KdlUtils.ParseUInt32(GetBufferedString(), radix, type), propName, source); return true;
            case "u64": EmitPropOrArg(KdlUtils.ParseUInt64(GetBufferedString(), radix, type), propName, source); return true;
            case "i8": EmitPropOrArg(KdlUtils.ParseInt8(GetBufferedString(), (sbyte)sign, radix, type), propName, source); return true;
            case "i16": EmitPropOrArg(KdlUtils.ParseInt16(GetBufferedString(), (short)sign, radix, type), propName, source); return true;
            case "i32": EmitPropOrArg(KdlUtils.ParseInt32(GetBufferedString(), sign, radix, type), propName, source); return true;
            case "i64": EmitPropOrArg(KdlUtils.ParseInt64(GetBufferedString(), sign, radix, type), propName, source); return true;
            case "isize": EmitPropOrArg(KdlUtils.ParseIntPtr(GetBufferedString(), sign, radix, type), propName, source); return true;
            case "usize": EmitPropOrArg(KdlUtils.ParseUIntPtr(GetBufferedString(), radix, type), propName, source); return true;
            case "f32":
                ThrowIf(radix != 10, "Invalid number. Floats must be base 10.");
                EmitPropOrArg(KdlUtils.ParseFloat32(GetBufferedString(), sign, type), propName, source);
                return true;
            case "f64":
            case "decimal64":
                ThrowIf(radix != 10, "Invalid number. Floats must be base 10.");
                EmitPropOrArg(KdlUtils.ParseFloat64(GetBufferedString(), sign, type), propName, source);
                return true;
            case "decimal":
            case "decimal128":
                ThrowIf(radix != 10, "Invalid number. Decimals must be base 10.");
                EmitPropOrArg(KdlUtils.ParseDecimal(GetBufferedString(), sign, type), propName, source);
                return true;
        }

        return false;
    }

    protected void ParseIntFromBuffer(int radix, int sign, string? type, string? propName, KdlSourceInfo source)
    {
        ThrowIf(_stringBuffer.Count == 0, "Invalid number. Number must have at least one digit.");

        if (type != null && _options.UseTypeAnnotations)
        {
            if (ParseNumWithType(radix, sign, type, propName, source))
                return;
        }

        try
        {
            KdlNumber<int> value = KdlUtils.ParseInt32(GetBufferedString(), sign, radix, type);
            EmitPropOrArg(value, propName, source);
        }
        catch (OverflowException)
        {
            try
            {
                KdlNumber<long> value = KdlUtils.ParseInt64(GetBufferedString(), sign, radix, type);
                EmitPropOrArg(value, propName, source);
            }
            catch (OverflowException)
            {
                if (sign == -1)
                {
                    throw new KdlException("Invalid number. Negative numbers must fit within a 64-bit signed integer.", GetSourceInfo());
                }

                KdlNumber<ulong> value = KdlUtils.ParseUInt64(GetBufferedString(), radix, type);
                EmitPropOrArg(value, propName, source);
            }
        }
    }

    protected void ParseHexNum(int sign, string? type, string? propName)
    {
        ThrowIfAtEnd();

        byte ch = _bytes[_offset];
        ThrowIf(ch == '_', "Invalid number. Numbers may not start with an underscore.");

        KdlSourceInfo source = GetSourceInfo();
        while (!AtEnd() && (KdlUtils.IsHexadecimalDigit(ch) || ch == '_'))
        {
            if (ch != '_')
            {
                _stringBuffer.Add(ch);
            }
            ++_offset;
            ch = _bytes[_offset];
        }

        ParseIntFromBuffer(16, sign, type, propName, source);
    }

    protected void ParseOctNum(int sign, string? type, string? propName)
    {
        ThrowIfAtEnd();

        byte ch = _bytes[_offset];
        ThrowIf(ch == '_', "Invalid number. Numbers may not start with an underscore.");

        KdlSourceInfo source = GetSourceInfo();
        while (!AtEnd() && (KdlUtils.IsOctalDigit(ch) || ch == '_'))
        {
            if (ch != '_')
            {
                _stringBuffer.Add(ch);
            }
            ++_offset;
            ch = _bytes[_offset];
        }

        ParseIntFromBuffer(8, sign, type, propName, source);
    }

    protected void ParseBinNum(int sign, string? type, string? propName)
    {
        ThrowIfAtEnd();

        byte ch = _bytes[_offset];
        ThrowIf(ch == '_', "Invalid number. Numbers may not start with an underscore.");

        KdlSourceInfo source = GetSourceInfo();
        while (!AtEnd() && (KdlUtils.IsBinaryDigit(ch) || ch == '_'))
        {
            if (ch != '_')
            {
                _stringBuffer.Add(ch);
            }
            ++_offset;
            ch = _bytes[_offset];
        }

        ParseIntFromBuffer(2, sign, type, propName, source);
    }

    protected void ParseDecNum(int sign, string? type, string? propName)
    {
        ThrowIfAtEnd();

        int dot = 0;
        int exp = 0;
        int expSign = 0;
        bool prevWasNumeric = false;
        KdlSourceInfo source = GetSourceInfo();

        while (!AtEnd())
        {
            int ucc = PeekCodePoint(out int len);

            if (!KdlUtils.IsFloatingDigit(ucc) && ucc != '_')
            {
                break;
            }

            switch (ucc)
            {
                case '.':
                {
                    ThrowIf(!prevWasNumeric, "Invalid number. Numbers must have a digit before the dot.");
                    ThrowIf(dot != 0, "Invalid number. Numbers cannot have multiple dots.");

                    dot = _stringBuffer.Count;
                    break;
                }
                case 'e':
                case 'E':
                {
                    ThrowIf(!prevWasNumeric, "Invalid number. Numbers must have a digit before the exponent.");
                    ThrowIf(exp != 0, "Invalid number. Numbers cannot have multiple exponents.");

                    exp = _stringBuffer.Count;
                    break;
                }
                case '+':
                case '-':
                {
                    ThrowIf(exp == 0 || expSign != 0, "Invalid number. Numbers cannot have multiple signs.");

                    expSign = _stringBuffer.Count;
                    break;
                }
                case '_':
                {
                    ThrowIf(_stringBuffer.Count == 0 || dot == (_stringBuffer.Count - 1), "Invalid number. Numbers may not start with an underscore.");
                    break;
                }
            }

            if (ucc != '_')
            {
                UTF8Encode(_stringBuffer, ucc);
            }

            prevWasNumeric = ucc >= '0' && ucc <= '9';
            _offset += len;
        }

        if (dot != 0 || exp != 0)
        {
            ThrowIf(_stringBuffer.Count == 0, "Invalid number. Numbers must have at least one digit.");

            ThrowIf(_stringBuffer[0] == '.' || _stringBuffer[0] == 'e' || _stringBuffer[0] == 'E',
                "Invalid number. Numbers may not start with a dot or exponent.");

            ThrowIf(_stringBuffer[^1] == '.' || _stringBuffer[^1] == 'e' || _stringBuffer[^1] == 'E' || _stringBuffer[^1] == '-' || _stringBuffer[^1] == '+',
                "Invalid number. Numbers may not end with a dot, exponent, or sign.");

            ThrowIf(exp != 0 && exp < dot, "Invalid number. Exponent must come after the dot.");

            try
            {
                KdlNumber<float> value = KdlUtils.ParseFloat32(GetBufferedString(), sign, type);
                if (type != null && _options.UseTypeAnnotations)
                {
                    if (ParseNumWithType(value, sign, type, propName, source))
                    {
                        return;
                    }
                }
                EmitPropOrArg(value, propName, source);
                return;
            }
            catch (OverflowException)
            {
                KdlNumber<double> value = KdlUtils.ParseFloat64(GetBufferedString(), sign, type);
                if (type != null && _options.UseTypeAnnotations)
                {
                    if (ParseNumWithType(value, sign, type, propName, source))
                    {
                        return;
                    }
                }
                EmitPropOrArg(value, propName, source);
                return;
            }
        }

        ParseIntFromBuffer(10, sign, type, propName, source);
    }

    protected void ParseNum(string? type, string? propName)
    {
        ThrowIfAtEnd();

        _stringBuffer.Clear();

        int sign = 1;
        int ucc = PeekCodePoint(out _);

        if (ucc == '-')
        {
            Consume('-');
            sign = -1;
        }
        else if (ucc == '+')
        {
            Consume('+');
        }

        ThrowIfAtEnd();

        ucc = PeekCodePoint(out int len);

        // 0x, 0o, 0b, 0, 0.0
        if (ucc == '0')
        {
            _offset += len;

            if (!AtEnd())
            {
                switch (_bytes[_offset])
                {
                    case (byte)'x': Consume('x'); ParseHexNum(sign, type, propName); return;
                    case (byte)'o': Consume('o'); ParseOctNum(sign, type, propName); return;
                    case (byte)'b': Consume('b'); ParseBinNum(sign, type, propName); return;
                }
            }

            // Back up the cursor so the leading zero is restored, and fall through to
            // decimal number parsing.
            _offset -= len;
        }

        ParseDecNum(sign, type, propName);
    }

    protected void ParseArgumentOrProperty(string? propName = null)
    {
        SkipSpaces();

        int ucc = PeekCodePoint(out int len);

        // Consume the type annotation if present
        string? type = null;
        if (ucc == '(')
        {
            type = ConsumeType();
            ucc = PeekCodePoint(out len);
        }

        switch (ucc)
        {
            // keyword value, or raw string
            // #true, #false, #null, #nan, #inf, #-inf, #"rawstr"#
            case '#':
            {
                KdlSourceInfo source = GetSourceInfo();

                // If the next character is a quote, or another hash, then this is a raw string
                if (_offset + 1 < _bytes.Length && (_bytes[_offset + 1] == '"' || _bytes[_offset + 1] == '#'))
                {
                    string value = ConsumeString();
                    EmitPropOrArg(value, type, propName, source);
                    return;
                }

                // otherwise, this is a keyword value
                _offset += len;
                switch (_bytes[_offset])
                {
                    case (byte)'t': Consume("true"); EmitPropOrArg(true, type, propName, source); return;
                    case (byte)'f': Consume("false"); EmitPropOrArg(false, type, propName, source); return;
                    case (byte)'i': Consume("inf"); EmitPropOrArg(double.PositiveInfinity, type, propName, source); return;
                    case (byte)'-': Consume("-inf"); EmitPropOrArg(double.NegativeInfinity, type, propName, source); return;
                    case (byte)'n':
                    {
                        if (_offset + 1 < _bytes.Length && _bytes[_offset + 1] == 'a')
                        {
                            Consume("nan");
                            EmitPropOrArg(double.NaN, type, propName, source);
                            return;
                        }

                        Consume("null");
                        EmitPropOrArg(null, type, propName, source);
                        return;
                    }
                }

                throw new KdlException($"Invalid token. Expected 'true', 'false', 'inf', '-inf', 'nan', or 'null' but found '{char.ConvertFromUtf32(ucc)}'.", GetSourceInfo());
            }
            // any digit means this is a number
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                ParseNum(type, propName);
                return;
            }
            // could be a number or the start of an identifier string
            case '-':
            case '+':
            {
                if (_offset + len < _bytes.Length && _bytes[_offset + len] != '\0' && KdlUtils.IsDecimalDigit(_bytes[_offset + len]))
                {
                    ParseNum(type, propName);
                    return;
                }

                // fall through to string parsing
                goto default;
            }
            // anything else must be a string
            default:
            {
                KdlSourceInfo source = GetSourceInfo();
                string value = ConsumeString();

                if (AtEnd())
                {
                    EmitPropOrArg(value, type, propName, source);
                    return;
                }

                int beforeSpaces = _offset;
                SkipSpaces(false);

                if (AtEnd())
                {
                    EmitPropOrArg(value, type, propName, source);
                    return;
                }

                ucc = PeekCodePoint(out len);

                if (KdlUtils.IsEqualsSign(ucc))
                {
                    ThrowIf(propName != null, "Invalid token. Multiple equals signs in property expression.");
                    ThrowIf(type != null, "Invalid token. Type annotations are not allowed on property names.");

                    _offset += len;
                    ParseArgumentOrProperty(value);
                    return;
                }

                // If not an equal sign, then we need to back up to before we tried to
                // parse this as a property. Otherwise the required spaces between args
                // and props will get consumed here.
                _offset = beforeSpaces;
                EmitPropOrArg(value, type, propName, source);
                return;
            }
        }
    }

    private static bool IsUTF8Continuation(byte ucc)
    {
        return (ucc & 0xC0) == 0x80;
    }

    private static int UTF8Encode(List<byte> bytes, int ucc)
    {
        // Top bit can't be set.
        if ((ucc & 0x80000000) != 0)
            return 0;

        // 6 possible encodings: http://en.wikipedia.org/wiki/UTF-8
        for (int i = 0; i < 6; ++i)
        {
            // Max bits this encoding can represent.
            int maxBits = 6 + (i * 5) + (i == 0 ? 1 : 0);

            if (ucc < (1u << maxBits))
            {
                // Remaining bits not encoded in the first byte, store 6 bits each
                int remainingBits = i * 6;

                // Store bytes
                bytes.Add((byte)((0xFE << (maxBits - remainingBits)) | (ucc >> remainingBits)));
                for (int j = i - 1; j != -1; --j)
                {
                    bytes.Add((byte)(((ucc >> (j * 6)) & 0x3F) | 0x80));
                }

                // Return the number of bytes written.
                return i + 1;
            }
        }

        return 0;
    }

    /// <summary>
    /// Converts the a valid UTF-8 bytes sequence into a unicode code point.
    /// </summary>
    /// <param name="dst">The destination code point to write to. Set to \ref InvalidCodePoint
    ///     if the sequence is invalid or incomplete.</param>
    /// <param name="str">The UTF-8 byte sequence to parse.</param>
    /// <returns>Returns the number of bytes parsed, if the sequence was valid. Zero (`0`) is
    ///     returned if more bytes are required, but the sequence is otherwise valid.
    ///     \ref InvalidCodePoint is returned if the sequence is invalid.</returns>
    private static int UTF8Decode(out int dst, byte[] bytes, int offset)
    {
        dst = InvalidCodePoint;

        if (offset >= bytes.Length)
        {
            return 0;
        }

        byte ch = bytes[offset];

        // ASCII code point
        if ((ch & 0x80) == 0)
        {
            dst = ch;
            return 1;
        }

        // 2-byte sequence
        if ((ch & 0xe0) == 0xc0)
        {
            if (offset + 1 >= bytes.Length)
            {
                return 0;
            }

            if (!IsUTF8Continuation(bytes[offset + 1]))
            {
                return InvalidCodePoint;
            }

            dst = ((ch & 0x1f) << 6) | (bytes[offset + 1] & 0x3f);
            return 2;
        }

        // 3-byte sequence
        if ((ch & 0xf0) == 0xe0)
        {
            if (offset + 2 >= bytes.Length)
            {
                return 0;
            }

            if (!IsUTF8Continuation(bytes[offset + 1]) || !IsUTF8Continuation(bytes[offset + 2]))
            {
                return InvalidCodePoint;
            }

            dst = ((ch & 0x0f) << 12) | ((bytes[offset + 1] & 0x3f) << 6) | (bytes[offset + 2] & 0x3f);
            return 3;
        }

        // 4-byte sequence
        if ((ch & 0xf8) == 0xf0)
        {
            if (offset + 3 >= bytes.Length)
            {
                return 0;
            }

            if (!IsUTF8Continuation(bytes[offset + 1]) || !IsUTF8Continuation(bytes[offset + 2]) || !IsUTF8Continuation(bytes[offset + 3]))
            {
                return InvalidCodePoint;
            }

            dst = ((ch & 0x07) << 18) | ((bytes[offset + 1] & 0x3f) << 12) | ((bytes[offset + 2] & 0x3f) << 6) | (bytes[offset + 3] & 0x3f);
            return 4;
        }

        return InvalidCodePoint;
    }
}
