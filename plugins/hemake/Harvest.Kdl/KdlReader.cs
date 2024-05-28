// Copyright Chad Engler

using Harvest.Kdl.Exceptions;
using Harvest.Kdl.Types;
using System.Numerics;
using System.Text;

namespace Harvest.Kdl;

public class KdlReader
{
    private readonly KdlReadOptions _options;

    private KdlReadContext? _context;
    private IKdlReadHandler? _handler;

    private bool _inWhitespaceEscape = false;
    private int _nodeDepth = 0;

    private List<int> _slashDashDepthStack = new();

    public KdlReader(KdlReadOptions? options = null) : base()
    {
        _options = options ?? new KdlReadOptions();
    }

    public void Read(Stream stream, IKdlReadHandler handler)
    {
        using StreamReader reader = new(stream, Encoding.UTF8);
        Read(new KdlReadContext(reader), handler);
    }

    public void Read(string str, IKdlReadHandler handler)
    {
        using MemoryStream stream = new(Encoding.UTF8.GetBytes(str));
        using StreamReader reader = new(stream, Encoding.UTF8);
        Read(new KdlReadContext(reader), handler);
    }

    protected void Read(KdlReadContext context, IKdlReadHandler handler)
    {
        _context = context;
        _handler = handler;
        _nodeDepth = 0;
        _inWhitespaceEscape = false;

        SkipBOM();

        while (!AtEnd())
        {
            ParseExpression();
        }
    }

    protected void ParseExpression()
    {
        SkipSpaces();

        if (_nodeDepth > 0)
        {
            ThrowIfAtEnd();
        }
        else if (AtEnd())
        {
            return;
        }

        int ucc = _context!.Peek();

        if (KdlUtils.IsNewline(ucc))
        {
            ConsumeNewLine();
            return;
        }

        switch (ucc)
        {
            // start of a comment
            case '/':
                ParseComment();
                break;

            // empty node terminated by semicolon
            case ';':
                _context.Read();
                break;

            // end of a node
            case '}':
                EmitEndNode();
                _context.Read();
                break;

            // start of type annotation or string name of node
            case '(':
            case '"':
            case '#':
                ParseNode();
                break;

            // start of an identifier string name of node
            default:
                if (KdlUtils.IsValidInitialIdentifierCharacter(ucc))
                {
                    ParseNode();
                }
                else
                {
                    throw new KdlException($"Invalid token. Expected node start but found '{(char)ucc}'.", _context);
                }
                break;
        }
    }

    protected void ParseNode()
    {
        SkipSpaces();

        int ucc = _context!.Peek();

        string? type = null;
        if (ucc == '(')
        {
            type = ConsumeType();
        }

        string name = ConsumeString();

        EmitStartNode(name, type);

        if (AtEnd())
        {
            EmitEndNode();
            return;
        }

        ucc = _context.Peek();

        if (!KdlUtils.IsWhitespace(ucc) && !KdlUtils.IsNewline(ucc) && ucc != ';' && ucc != '/' && ucc != '}')
        {
            throw new KdlException($"Invalid token. Expected whitespace, newline, semicolon, comment, or end of node but found '{(char)ucc}'.", _context);
        }

        bool hasWhitespace = false;
        while (!AtEnd())
        {
            ucc = _context.Peek();

            // Newline is a node terminator, end the node and then consume it.
            if (KdlUtils.IsNewline(ucc))
            {
                EmitEndNode();
                ConsumeNewLine();
                return;
            }

            // Whitespace is required between the node name and the first argument or property
            if (KdlUtils.IsWhitespace(ucc))
            {
                hasWhitespace = true;
                SkipSpaces();
                continue;
            }

            // consume an argument, property, or start of children
            switch (ucc)
            {
                // single-line comment is a node terminator, end the node and then consume it.
                // multi-line and slashdash comments are allowed in the node
                case '/':
                {
                    _context.Read();
                    ThrowIfAtEnd();

                    if (_context.Peek() == '/' && !_inWhitespaceEscape)
                    {
                        EmitEndNode();
                        ParseComment();
                        return;
                    }

                    _context.Unread('/');
                    ParseComment();
                    break;
                }
                // semicolon is a node terminator, end the node and then consume it.
                case ';':
                {
                    EmitEndNode();
                    _context.Read();
                    return;
                }
                // end brace is a node terminator, end the node but don't consume it.
                // Let ParseExpression consume it to end the parent node.
                case '}':
                {
                    EmitEndNode();
                    return;
                }
                // open brace indicates we're starting a child block
                case '{':
                {
                    if (!hasWhitespace)
                    {
                        throw new KdlException("Invalid token. Expected whitespace before child block.", _context);
                    }

                    _context.Read();

                    int depth = _nodeDepth;
                    while (_nodeDepth >= depth)
                    {
                        ThrowIfAtEnd();
                        ParseExpression();
                    }

                    // Note: node end will be emitted by ParseExpression.
                    return;
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
                    if (!hasWhitespace)
                    {
                        throw new KdlException("Invalid token. Expected whitespace before a property or argument.", _context);
                    }

                    ParseArgumentOrProperty();
                    hasWhitespace = false;
                    break;
                }
                // anything else must be an identifier string
                // which could be an argument value or property name
                default:
                {
                    if (!hasWhitespace)
                    {
                        throw new KdlException("Invalid token. Expected whitespace before a property or argument.", _context);
                    }

                    if (!KdlUtils.IsValidInitialIdentifierCharacter(ucc))
                    {
                        throw new KdlException("Invalid token. Expected an initial identifier character.", _context);
                    }

                    ParseArgumentOrProperty();
                    hasWhitespace = false;
                    break;
                }
            }
        }

        // End the node if we reach EOF
        EmitEndNode();
    }

    protected void ParseComment()
    {
        Consume('/');
        ThrowIfAtEnd();

        int ucc = _context!.Peek();

        switch (ucc)
        {
            // slashdash
            case '-':
            {
                _context.Read();
                _slashDashDepthStack.Add(_nodeDepth + 1);
                _handler!.StartDocument();
                return;
            }
            // single-line comment
            case '/':
            {
                _context.Read();
                SkipSpaces();

                StringBuilder str = new(64);
                while (!AtEnd())
                {
                    if (KdlUtils.IsNewline(_context.Peek()))
                        break;

                    str.Append((char)_context.Read());
                }

                _handler!.Comment(str.ToString());

                if (!AtEnd())
                {
                    ConsumeNewLine();
                }
                break;
            }
            // multi-line comment
            case '*':
            {
                _context.Read();
                SkipSpaces();

                int prevUcc = 0;
                int depth = 1;
                StringBuilder str = new(128);
                while (depth > 0)
                {
                    ThrowIfAtEnd();

                    ucc = _context.Read();
                    if (ucc == '*' && prevUcc == '/')
                    {
                        ++depth;
                        str.Append((char)ucc);
                        ucc = 0; // reset so "/*/" isn't self-closing
                    }
                    else if (ucc == '/' && prevUcc == '*')
                    {
                        --depth;
                        if (depth > 0)
                        {
                            str.Append((char)ucc);
                        }
                        ucc = 0; // reset so "*/*" isn't self-opening
                    }
                    else
                    {
                        str.Append((char)ucc);
                    }

                    prevUcc = ucc;
                }

                _handler!.Comment(str.ToString());
                break;
            }
            default:
                throw new KdlException($"Invalid token. Expected '-', '/', or '*' but found '{(char)ucc}'.", _context);
        }
    }

    protected bool ParseNumWithType<T>(KdlNumber<T> num, int sign, string type, string? propName) where T : INumber<T>
    {
        switch (type)
        {
            case "u8":
            case "u16":
            case "u32":
            case "u64":
            case "usize":
                if (sign != 1)
                {
                    throw new KdlException("Invalid number. Unsigned types cannot be negative.", _context);
                }
                break;
        }

        switch (type)
        {
            case "u8": EmitPropOrArg(num.Value is byte ? num : new KdlNumber<byte>((byte)Convert.ChangeType(num.Value, typeof(byte)), num.Radix, num.Type), propName); return true;
            case "u16": EmitPropOrArg(num.Value is ushort ? num : new KdlNumber<ushort>((ushort)Convert.ChangeType(num.Value, typeof(ushort)), num.Radix, num.Type), propName); return true;
            case "u32": EmitPropOrArg(num.Value is uint ? num : new KdlNumber<uint>((uint)Convert.ChangeType(num.Value, typeof(uint)), num.Radix, num.Type), propName); return true;
            case "u64": EmitPropOrArg(num.Value is ulong ? num : new KdlNumber<ulong>((ulong)Convert.ChangeType(num.Value, typeof(ulong)), num.Radix, num.Type), propName); return true;
            case "i8": EmitPropOrArg(num.Value is sbyte ? num : new KdlNumber<sbyte>((sbyte)Convert.ChangeType(num.Value, typeof(sbyte)), num.Radix, num.Type), propName); return true;
            case "i16": EmitPropOrArg(num.Value is short ? num : new KdlNumber<short>((short)Convert.ChangeType(num.Value, typeof(short)), num.Radix, num.Type), propName); return true;
            case "i32": EmitPropOrArg(num.Value is int ? num : new KdlNumber<int>((int)Convert.ChangeType(num.Value, typeof(int)), num.Radix, num.Type), propName); return true;
            case "i64": EmitPropOrArg(num.Value is long ? num : new KdlNumber<long>((long)Convert.ChangeType(num.Value, typeof(long)), num.Radix, num.Type), propName); return true;
            case "isize": EmitPropOrArg(num.Value is nint ? num : new KdlNumber<nint>((nint)Convert.ChangeType(num.Value, typeof(nint)), num.Radix, num.Type), propName); return true;
            case "usize": EmitPropOrArg(num.Value is nuint ? num : new KdlNumber<nuint>((nuint)Convert.ChangeType(num.Value, typeof(nuint)), num.Radix, num.Type), propName); return true;
            case "f32": EmitPropOrArg(num.Value is float ? num : new KdlNumber<float>((float)Convert.ChangeType(num.Value, typeof(float)), num.Radix, num.Type), propName); return true;
            case "f64": EmitPropOrArg(num.Value is double ? num : new KdlNumber<double>((double)Convert.ChangeType(num.Value, typeof(double)), num.Radix, num.Type), propName); return true;
            case "decimal64": EmitPropOrArg(num.Value is double ? num : new KdlNumber<double>((double)Convert.ChangeType(num.Value, typeof(double)), num.Radix, num.Type), propName); return true;
            case "decimal": EmitPropOrArg(num.Value is decimal ? num : new KdlNumber<decimal>((decimal)Convert.ChangeType(num.Value, typeof(decimal)), num.Radix, num.Type), propName); return true;
            case "decimal128": EmitPropOrArg(num.Value is decimal ? num : new KdlNumber<decimal>((decimal)Convert.ChangeType(num.Value, typeof(decimal)), num.Radix, num.Type), propName); return true;
        }

        return false;
    }

    protected bool ParseNumWithType(StringBuilder builder, int radix, int sign, string type, string? propName)
    {
        switch (type)
        {
            case "u8":
            case "u16":
            case "u32":
            case "u64":
            case "usize":
                if (sign != 1)
                {
                    throw new KdlException("Invalid number. Unsigned types cannot be negative.", _context);
                }
                break;
        }

        switch (type)
        {
            case "u8": EmitPropOrArg(KdlUtils.ParseUInt8(builder.ToString(), radix, type), propName); return true;
            case "u16": EmitPropOrArg(KdlUtils.ParseUInt16(builder.ToString(), radix, type), propName); return true;
            case "u32": EmitPropOrArg(KdlUtils.ParseUInt32(builder.ToString(), radix, type), propName); return true;
            case "u64": EmitPropOrArg(KdlUtils.ParseUInt64(builder.ToString(), radix, type), propName); return true;
            case "i8": EmitPropOrArg(KdlUtils.ParseInt8(builder.ToString(), (sbyte)sign, radix, type), propName); return true;
            case "i16": EmitPropOrArg(KdlUtils.ParseInt16(builder.ToString(), (short)sign, radix, type), propName); return true;
            case "i32": EmitPropOrArg(KdlUtils.ParseInt32(builder.ToString(), sign, radix, type), propName); return true;
            case "i64": EmitPropOrArg(KdlUtils.ParseInt64(builder.ToString(), sign, radix, type), propName); return true;
            case "isize": EmitPropOrArg(KdlUtils.ParseIntPtr(builder.ToString(), sign, radix, type), propName); return true;
            case "usize": EmitPropOrArg(KdlUtils.ParseUIntPtr(builder.ToString(), radix, type), propName); return true;
            case "f32":
                if (radix != 10)
                {
                    throw new KdlException("Invalid number. Floats must be base 10.", _context);
                }
                EmitPropOrArg(KdlUtils.ParseFloat32(builder.ToString(), sign, type), propName);
                return true;
            case "f64":
            case "decimal64":
                if (radix != 10)
                {
                    throw new KdlException("Invalid number. Floats must be base 10.", _context);
                }
                EmitPropOrArg(KdlUtils.ParseFloat64(builder.ToString(), sign, type), propName);
                return true;
            case "decimal":
            case "decimal128":
                if (radix != 10)
                {
                    throw new KdlException("Invalid number. Decimals must be base 10.", _context);
                }
                EmitPropOrArg(KdlUtils.ParseDecimal(builder.ToString(), sign, type), propName);
                return true;
        }

        return false;
    }

    protected void ParseIntFromBuilder(StringBuilder builder, int radix, int sign, string? type, string? propName)
    {
        if (builder.Length == 0)
        {
            throw new KdlException("Invalid number. Number must have at least one digit.", _context);
        }

        if (type != null && _options.UseTypeAnnotations)
        {
            if (ParseNumWithType(builder, radix, sign, type, propName))
                return;
        }

        try
        {
            KdlNumber<int> value = KdlUtils.ParseInt32(builder.ToString(), sign, radix, type);
            EmitPropOrArg(value, propName);
        }
        catch (OverflowException)
        {
            try
            {
                KdlNumber<long> value = KdlUtils.ParseInt64(builder.ToString(), sign, radix, type);
                EmitPropOrArg(value, propName);
            }
            catch (OverflowException)
            {
                if (sign == -1)
                {
                    throw new KdlException("Invalid number. Huge negative numbers are not supported.", _context);
                }

                KdlNumber<ulong> value = KdlUtils.ParseUInt64(builder.ToString(), radix, type);
                EmitPropOrArg(value, propName);
            }
        }
    }

    protected void ParseHexNum(int sign, string? type, string? propName)
    {
        ThrowIfAtEnd();

        int ucc = _context!.Peek();
        if (ucc == '_')
        {
            throw new KdlException("Invalid number. Numbers may not start with an underscore.", _context);
        }

        StringBuilder builder = new();
        while (!AtEnd() && (KdlUtils.IsHexadecimalDigit(ucc) || ucc == '_'))
        {
            if (ucc != '_')
                builder.Append((char)ucc);
            _context.Read();
        }

        ParseIntFromBuilder(builder, 16, sign, type, propName);
    }

    protected void ParseOctNum(int sign, string? type, string? propName)
    {
        ThrowIfAtEnd();

        int ucc = _context!.Peek();
        if (ucc == '_')
        {
            throw new KdlException("Invalid number. Numbers may not start with an underscore.", _context);
        }

        StringBuilder builder = new();
        while (!AtEnd() && (KdlUtils.IsOctalDigit(ucc) || ucc == '_'))
        {
            if (ucc != '_')
                builder.Append((char)ucc);
            _context.Read();
        }

        ParseIntFromBuilder(builder, 8, sign, type, propName);
    }

    protected void ParseBinNum(int sign, string? type, string? propName)
    {
        ThrowIfAtEnd();

        int ucc = _context!.Peek();
        if (ucc == '_')
        {
            throw new KdlException("Invalid number. Numbers may not start with an underscore.", _context);
        }

        StringBuilder builder = new();
        while (!AtEnd() && (KdlUtils.IsBinaryDigit(ucc) || ucc == '_'))
        {
            if (ucc != '_')
                builder.Append((char)ucc);
            _context.Read();
        }

        ParseIntFromBuilder(builder, 2, sign, type, propName);
    }

    protected void ParseDecNum(int sign, string? type, string? propName)
    {
        ThrowIfAtEnd();

        int dot = 0;
        int exp = 0;
        int expSign = 0;
        bool prevWasNumeric = false;
        StringBuilder builder = new();

        while (!AtEnd())
        {
            int ucc = _context!.Peek();

            if (!KdlUtils.IsFloatingDigit(ucc) && ucc != '_')
                break;

            switch (ucc)
            {
                case '.':
                {
                    if (!prevWasNumeric)
                    {
                        throw new KdlException("Invalid number. Numbers must have a digit before the dot.", _context);
                    }

                    if (dot != 0)
                    {
                        throw new KdlException("Invalid number. Numbers cannot have multiple dots.", _context);
                    }

                    dot = builder.Length;
                    break;
                }
                case 'e':
                case 'E':
                {
                    if (!prevWasNumeric)
                    {
                        throw new KdlException("Invalid number. Numbers must have a digit before the exponent.", _context);
                    }

                    if (exp != 0)
                    {
                        throw new KdlException("Invalid number. Numbers cannot have multiple exponents.", _context);
                    }

                    exp = builder.Length;
                    break;
                }
                case '+':
                case '-':
                {
                    if (exp == 0 || expSign != 0)
                    {
                        throw new KdlException("Invalid number. Numbers cannot have multiple signs.", _context);
                    }

                    expSign = builder.Length;
                    break;
                }
                case '_':
                {
                    if (builder.Length == 0 || dot == (builder.Length - 1))
                    {
                        throw new KdlException("Invalid number. Numbers may not start with an underscore.", _context);
                    }
                    break;
                }
            }

            if (ucc != '_')
            {
                builder.Append((char)ucc);
            }

            prevWasNumeric = ucc >= '0' && ucc <= '9';
            _context.Read();
        }

        if (dot != 0 || exp != 0)
        {
            if (builder.Length == 0)
            {
                throw new KdlException("Invalid number. Numbers must have at least one digit.", _context);
            }

            // Cannot start with a dot or exponent
            if (builder[0] == '.' || builder[0] == 'e' || builder[0] == 'E')
            {
                throw new KdlException("Invalid number. Numbers may not start with a dot or exponent.", _context);
            }

            // Cannot end with a dot, exponent, or sign
            if (builder[^1] == '.' || builder[^1] == 'e' || builder[^1] == 'E' || builder[^1] == '-' || builder[^1] == '+')
            {
                throw new KdlException("Invalid number. Numbers may not end with a dot, exponent, or sign.", _context);
            }

            // exponent cannot be before the dot
            if (exp != 0 && exp < dot)
            {
                throw new KdlException("Invalid number. Exponent must come after the dot.", _context);
            }

            KdlNumber<double> value = KdlUtils.ParseFloat64(builder.ToString(), sign, type);

            if (type != null && _options.UseTypeAnnotations)
            {
                if (ParseNumWithType(value, sign, type, propName))
                    return;
            }

            EmitPropOrArg(value, propName);
        }
        else
        {
            ParseIntFromBuilder(builder, 10, sign, type, propName);
        }
    }


    protected void ParseNum(string? type, string? propName)
    {
        ThrowIfAtEnd();

        int sign = 1;
        int ucc = _context!.Peek();

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

        ucc = _context.Peek();

        // 0x, 0o, 0b, 0, 0.0
        if (ucc == '0')
        {
            Consume('0');

            if (!AtEnd())
            {
                switch (_context.Peek())
                {
                    case 'x': Consume('x'); ParseHexNum(sign, type, propName); return;
                    case 'o': Consume('o'); ParseOctNum(sign, type, propName); return;
                    case 'b': Consume('b'); ParseBinNum(sign, type, propName); return;
                }
            }

            // Back up the cursor so the leading zero is restored, and fall through to
            // decimal number parsing.
            _context.Unread('0');
        }

        ParseDecNum(sign, type, propName);
    }

    protected void ParseArgumentOrProperty(string? propName = null)
    {
        SkipSpaces();

        int ucc = _context!.Peek();

        // Consume the type annotation if present
        string? type = null;
        if (ucc == '(')
        {
            type = ConsumeType();
        }

        switch (ucc)
        {
            // keyword value, or raw string
            // #true, #false, #null, #nan, #inf, #-inf, #"rawstr"#
            case '#':
            {
                _context.Read();
                ThrowIfAtEnd();

                ucc = _context.Peek();

                // If the next character is a quote, or another hash, then this is a raw string
                if (ucc == '"' || ucc == '#')
                {
                    string value = ConsumeString();
                    EmitPropOrArg(value, type, propName);
                    return;
                }

                // otherwise, this is a keyword value
                switch (ucc)
                {
                    case 't': Consume("true"); EmitPropOrArg(true, type, propName); return;
                    case 'f': Consume("false"); EmitPropOrArg(false, type, propName); return;
                    case 'i': Consume("inf"); EmitPropOrArg(double.PositiveInfinity, type, propName); return;
                    case '-': Consume("-inf"); EmitPropOrArg(double.NegativeInfinity, type, propName); return;
                    case 'n':
                    {
                        _context.Read();
                        ThrowIfAtEnd();

                        ucc = _context.Peek();
                        _context.Unread('n');

                        if (ucc == 'a')
                        {
                            Consume("nan");
                            EmitPropOrArg(double.NaN, type, propName);
                        }
                        else
                        {
                            Consume("null");
                            EmitPropOrArg(null, type, propName);
                        }
                        return;
                    }
                }

                throw new KdlException($"Invalid token. Expected 'true', 'false', 'inf', '-inf', 'nan', or 'null' but found '{(char)ucc}'.", _context);
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
                ParseNum(type, propName);
                return;
            // could be a number or the start of an identifier string
            case '-':
            case '+':
            {
                int sign = ucc;
                _context.Read();
                if (!AtEnd() && KdlUtils.IsDecimalDigit(_context.Peek()))
                {
                    _context.Unread(sign);
                    ParseNum(type, propName);
                    return;
                }

                // fall through to string parsing
                _context.Unread(sign);
                goto default;
            }
            // anything else must be a string
            default:
            {
                string value = ConsumeString();

                if (AtEnd())
                {
                    EmitPropOrArg(value, type, propName);
                    return;
                }

                SkipSpaces(false);

                if (AtEnd())
                {
                    EmitPropOrArg(value, type, propName);
                    return;
                }

                ucc = _context.Peek();

                if (KdlUtils.IsEqualsSign(ucc))
                {
                    if (propName != null)
                    {
                        throw new KdlException("Invalid token. Multiple equals signs in property expression.", _context);
                    }

                    if (type != null)
                    {
                        throw new KdlException("Invalid token. Type annotations are not allowed on property names.", _context);
                    }

                    _context.Read();
                    ParseArgumentOrProperty(value);
                    return;
                }

                // If not an equal sign, then we need to back up to before we tried to
                // parse this as a property. Otherwise the required spaces between args
                // and props will get consumed here.
                _context.Unread(' ');
                EmitPropOrArg(value, type, propName);
                return;
            }
        }
    }

    protected bool AtEnd()
    {
        return _context!.Peek() == KdlUtils.EOF;
    }

    protected void ThrowIfAtEnd()
    {
        if (AtEnd())
        {
            throw new KdlException("Unexpected end of file.", _context);
        }
    }

    protected void SkipBOM()
    {
        if (_context!.Peek() == KdlUtils.BOM)
        {
            _context!.Read();
        }
    }

    protected void SkipSpaces(bool allowSlashdash = true)
    {
        while (!AtEnd())
        {
            int ucc = _context!.Peek();

            // catch escape sequences for escaped newlines
            if (ucc == '\\')
            {
                _inWhitespaceEscape = true;
                _context.Read();
            }
            // anywhere you can have whitespace, you can also have comments
            else if (ucc == '/')
            {
                ucc = _context.Peek();

                // Single line comments and slashdash comments are terminators for nodes, they don't count just "spaces"
                if (ucc == '/')
                    return;

                // Some contexts don't allow slashdash comments
                if (!allowSlashdash && ucc == '-')
                    return;

                ParseComment();
            }
            // if it is whitespace consume it and continue
            else if (KdlUtils.IsWhitespace(ucc))
            {
                _context.Read();
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

            _context.Read();
        }
    }

    protected void CheckSlashdashEnd()
    {
        if (_slashDashDepthStack.Count > 0 && _slashDashDepthStack[^1] == _nodeDepth + 1)
        {
            _slashDashDepthStack.RemoveAt(_slashDashDepthStack.Count - 1);
            _handler!.EndComment();
        }
    }

    protected void EmitStartNode(string name, string? type)
    {
        _handler!.StartNode(name, type);
        ++_nodeDepth;
    }

    protected void EmitEndNode()
    {
        if (_nodeDepth == 0)
        {
            throw new KdlException("Invalid document. Unexpected end of node.", _context);
        }

        _handler!.EndNode();
        --_nodeDepth;
        CheckSlashdashEnd();
    }

    protected void EmitPropOrArg(KdlValue kdlValue, string? name)
    {
        if (name != null)
        {
            _handler!.Property(name, kdlValue);
        }
        else
        {
            _handler!.Argument(kdlValue);
        }

        CheckSlashdashEnd();
    }

    protected void EmitPropOrArg(object? value, string? type, string? name)
    {
        KdlValue kdlValue = KdlValue.From(value, type);
        EmitPropOrArg(kdlValue, type, name);
    }

    protected void Consume(char ch)
    {
        ThrowIfAtEnd();

        int ucc = _context!.Peek();

        if (ucc != ch)
        {
            throw new KdlException($"Invalid token. Expected '{ch}' but found '{(char)ucc}'.", _context);
        }

        _context.Read();
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

        int ucc = _context!.Peek();

        if (!KdlUtils.IsNewline(ucc))
        {
            throw new KdlException($"Invalid token. Expected newline but found '{(char)ucc}'.", _context);
        }

        _context.Read();
    }

    protected string ConsumeType()
    {
        Consume('(');
        SkipSpaces();
        string type = ConsumeString();
        SkipSpaces();
        Consume(')');
        SkipSpaces();

        return type;
    }

    protected string ConsumeUnicodeEscapeSequence()
    {
        Consume('u');
        Consume('{');
        ThrowIfAtEnd();

        // Max of 6 hex characters
        bool foundNum = false;
        int ucc = 0;
        for (int i = 0; i < 6; ++i)
        {
            if (AtEnd())
                break;

            int ch = _context!.Peek();

            if (ch == '}')
                break;

            if (!KdlUtils.IsHexadecimalDigit(ch))
            {
                throw new KdlException($"Invalid token. Expected hex digit but found '{(char)ch}'.", _context);
            }

            ucc = (ucc << 4) + KdlUtils.HexToNibble((char)ch);
            foundNum = true;
            _context.Read();
        }

        Consume('}');

        if (!foundNum)
        {
            throw new KdlException("Invalid unicode escape sequence. No hex digits found in braces.", _context);
        }

        if (!KdlUtils.IsUnicodeScalarValue(ucc))
        {
            throw new KdlException("Invalid unicode escape sequence. Value is not a valid unicode scalar value.", _context);
        }

        return char.ConvertFromUtf32(ucc);
    }

    protected string ConsumeQuotedString(int rawDelimCount)
    {
        bool firstChar = true;
        bool isMultiline = false;
        bool inEscapeSeq = false;
        bool inWhitespaceEscape = false;
        bool isTerminated = false;
        StringBuilder value = new(64);
        while (!AtEnd())
        {
            int ucc = _context!.Peek();

            // Multi-line string handling
            if (firstChar)
            {
                isMultiline = KdlUtils.IsNewline(ucc);
                firstChar = false;

                if (isMultiline)
                {
                    ConsumeNewLine();
                    continue;
                }
            }

            // Escape sequence handling
            if (inEscapeSeq)
            {
                // skip whitespace after escape sequence
                if (KdlUtils.IsWhitespace(ucc))
                {
                    inWhitespaceEscape = true;
                    _context.Read();
                    continue;
                }

                // skip newlines after escape sequence
                if (KdlUtils.IsNewline(ucc))
                {
                    inWhitespaceEscape = true;
                    ConsumeNewLine();
                    //ConsumeDedentPrefix(dedentPrefix);
                    continue;
                }

                if (inWhitespaceEscape)
                {
                    inEscapeSeq = false;
                    inWhitespaceEscape = false;
                    continue;
                }

                if (ucc == 'u')
                {
                    value.Append(ConsumeUnicodeEscapeSequence());
                    inEscapeSeq = false;
                    continue;
                }

                switch (ucc)
                {
                    case 'n': value.Append('\n'); break;
                    case 'r': value.Append('\r'); break;
                    case 't': value.Append('\t'); break;
                    case '\\': value.Append('\\'); break;
                    case '"': value.Append('"'); break;
                    case 'b': value.Append('\b'); break;
                    case 'f': value.Append('\f'); break;
                    case 's': value.Append(' '); break;
                    default:
                        throw new KdlException($"Invalid escape sequence '\\{(char)ucc}'.", _context);
                }

                inEscapeSeq = false;
                _context.Read();
                continue;
            }

            // Newline handling is different for multi-line or single-line strings
            if (KdlUtils.IsNewline(ucc))
            {
                // unescaped newlines are not allowed in single-line strings
                if (!isMultiline)
                {
                    throw new KdlException("Invalid token. Unescaped newlines not allowed in single-line strings.", _context);
                }

                ConsumeNewLine();

                // Consume the required dedent prefix since we consumed a newline
                //ConsumeDedentPrefix(dedentPrefix);

                // newlines are normalized to `\n`
                value.Append('\n');
                continue;
            }

            // New escape sequence start handling
            if (ucc == '\\')
            {
                if (rawDelimCount > 0)
                    value.Append('\\');
                else
                    inEscapeSeq = true;

                _context.Read();
                continue;
            }

            // End of string handling
            if (ucc == '"')
            {
                if (rawDelimCount > 0)
                {
                    int count = 0;
                    while (count < rawDelimCount && !AtEnd())
                    {
                        ucc = _context.Peek();

                        if (ucc == '#')
                        {
                            _context.Read();
                            ++count;
                        }
                        else
                        {
                            value.Append('"');
                            value.Append('#', count);
                            break;
                        }
                    }

                    if (count == rawDelimCount)
                    {
                        if (_context.Peek() == '#')
                        {
                            throw new KdlException("Invalid token. Expected end of raw string but found another delimiter.", _context);
                        }

                        isTerminated = true;
                        break;
                    }
                }
                else
                {
                    _context.Read();
                    isTerminated = true;
                    break;
                }
            }
            else
            {
                // Append the character to the buffer
                value.Append((char)_context.Read());
            }
        }

        if (!isTerminated)
        {
            throw new KdlException("Unexpected end of file. String was not properly terminated.", _context);
        }

        if (value.Length == 0)
            return string.Empty;

        if (!isMultiline)
            return value.ToString();

        // Calculate the indent prefix used on the last line of the string
        int indentPrefixStart = value.Length - 1;
        while (indentPrefixStart >= 0 && KdlUtils.IsWhitespace(value[indentPrefixStart]))
        {
            --indentPrefixStart;
        }

        if (indentPrefixStart < 0)
            return value.ToString();

        // Create a new value with the indent prefix removed
        StringBuilder newValue = new(value.Length);
        int indentIndex = 0;
        int indentLength = value.Length - indentPrefixStart;
        for (int i = 0; i < value.Length; ++i)
        {
            if (indentIndex < indentLength)
            {
                if (value[i] != value[indentPrefixStart + indentIndex])
                {
                    throw new KdlException("Invalid token. Each line of a multi-line string must have an identical indent.", _context);
                }

                ++indentIndex;
                continue;
            }

            if (KdlUtils.IsNewline(value[i]))
            {
                indentIndex = 0;
            }

            newValue.Append(value[i]);
        }

        return newValue.ToString();
    }

    protected string ConsumeIdentifierString()
    {
        StringBuilder value = new(32);

        bool first = true;
        while (!AtEnd())
        {
            int ucc = _context!.Peek();

            if (first && !KdlUtils.IsValidInitialIdentifierCharacter(ucc))
            {
                throw new KdlException($"Invalid token. Expected initial identifier character but found '{(char)ucc}'.", _context);
            }

            first = false;

            if (!KdlUtils.IsValidIdentifierCharacter(ucc))
                break;

            value.Append((char)_context.Read());
        }

        // No identifier string can start with a number
        if (KdlUtils.IsDecimalDigit(value[0]))
        {
            throw new KdlException("Invalid identifier. Identifier strings cannot start with a number.", _context);
        }

        if (value[0] == '-' || value[0] == '+')
        {
            // Identifiers can start with "-."/"+." as long as the next char is not a number
            if (value.Length > 2 && value[1] == '.' && KdlUtils.IsDecimalDigit(value[2]))
            {
                throw new KdlException("Invalid identifier. Identifier strings cannot start with a dash ('-') or plus ('+'), followed by a dot ('.') and a number.", _context);
            }

            // Identifiers can start with "-"/"+" as long as the next char is not a number
            if (value.Length > 1 && KdlUtils.IsDecimalDigit(value[1]))
            {
                throw new KdlException("Invalid identifier. Identifier strings cannot start with a dash ('-') or plus ('+'), followed by a number.", _context);
            }
        }

        if (value[0] == '.')
        {
            // Identifiers can start with "." as long as the next char is not a number
            if (value.Length > 1 && KdlUtils.IsDecimalDigit(value[1]))
            {
                throw new KdlException("Invalid identifier. Identifier strings cannot start with a dot ('.'), followed by a number.", _context);
            }
        }

        string str = value.ToString();

        // Identifiers can't be any of the language keywords without their leading '#'
        if (str == "inf" || str == "-inf" || str == "nan" || str == "true" || str == "false" || str == "null")
        {
            throw new KdlException("Invalid identifier. Identifier strings cannot be a language keyword.", _context);
        }

        return str;
    }

    protected string ConsumeString()
    {
        ThrowIfAtEnd();

        int ucc = _context!.Peek();

        if (ucc == '"')
        {
            _context.Read();
            return ConsumeQuotedString(0);
        }

        if (ucc == '#')
        {
            _context.Read();
            int rawDelimCount = 1;
            while (!AtEnd())
            {
                ucc = _context.Read();

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
}
