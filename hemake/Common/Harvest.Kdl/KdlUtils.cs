// Copyright Chad Engler

using Harvest.Kdl.Types;
using System.Globalization;
using System.Numerics;

namespace Harvest.Kdl;

internal static class KdlUtils
{
    public static readonly int EOF = -1;
    public static readonly int BOM = 0xFEFF;
    public static readonly int MinUnicode = 0x20;
    public static readonly int MaxUnicode = 0x10FFFF;
    public static readonly CultureInfo Culture = CultureInfo.GetCultureInfo("en-US");

    public static KdlNumber<float> ParseFloat32(string input, float sign, string? type)
    {
        float value = Convert.ToSingle(input, Culture);
        return new KdlNumber<float>((float)(value * sign), type);
    }

    public static KdlNumber<double> ParseFloat64(string input, double sign, string? type)
    {
        double value = Convert.ToDouble(input, Culture);
        return new KdlNumber<double>((double)(value * sign), type);
    }

    public static KdlNumber<decimal> ParseDecimal(string input, decimal sign, string? type)
    {
        NumberStyles styles = NumberStyles.AllowLeadingSign | NumberStyles.AllowDecimalPoint | NumberStyles.AllowExponent;
        decimal value = decimal.Parse(input, styles, Culture);
        return new KdlNumber<decimal>((decimal)(value * sign), type);
    }

    public static KdlNumber<sbyte> ParseInt8(string input, sbyte sign, int radix, string? type)
    {
        ulong magnitude = ParseUnsignedMagnitude64(input, radix);
        sbyte value;
        if (sign >= 0)
        {
            if (magnitude > (ulong)sbyte.MaxValue)
            {
                throw new OverflowException();
            }

            value = (sbyte)magnitude;
        }
        else
        {
            if (magnitude > (ulong)sbyte.MaxValue + 1)
            {
                throw new OverflowException();
            }

            value = magnitude == (ulong)sbyte.MaxValue + 1
                ? sbyte.MinValue
                : (sbyte)(-(long)magnitude);
        }

        return new KdlNumber<sbyte>(value, radix, type);
    }

    public static KdlNumber<short> ParseInt16(string input, short sign, int radix, string? type)
    {
        ulong magnitude = ParseUnsignedMagnitude64(input, radix);
        short value;
        if (sign >= 0)
        {
            if (magnitude > (ulong)short.MaxValue)
            {
                throw new OverflowException();
            }

            value = (short)magnitude;
        }
        else
        {
            if (magnitude > (ulong)short.MaxValue + 1)
            {
                throw new OverflowException();
            }

            value = magnitude == (ulong)short.MaxValue + 1
                ? short.MinValue
                : (short)(-(long)magnitude);
        }

        return new KdlNumber<short>(value, radix, type);
    }

    public static KdlNumber<int> ParseInt32(string input, int sign, int radix, string? type)
    {
        ulong magnitude = ParseUnsignedMagnitude64(input, radix);
        int value;
        if (sign >= 0)
        {
            if (magnitude > int.MaxValue)
            {
                throw new OverflowException();
            }

            value = (int)magnitude;
        }
        else
        {
            if (magnitude > (ulong)int.MaxValue + 1)
            {
                throw new OverflowException();
            }

            value = magnitude == (ulong)int.MaxValue + 1
                ? int.MinValue
                : (int)(-(long)magnitude);
        }

        return new KdlNumber<int>(value, radix, type);
    }

    public static KdlNumber<long> ParseInt64(string input, int sign, int radix, string? type)
    {
        ulong magnitude = ParseUnsignedMagnitude64(input, radix);
        long value;
        if (sign >= 0)
        {
            if (magnitude > long.MaxValue)
            {
                throw new OverflowException();
            }

            value = (long)magnitude;
        }
        else
        {
            if (magnitude > (ulong)long.MaxValue + 1)
            {
                throw new OverflowException();
            }

            value = magnitude == (ulong)long.MaxValue + 1
                ? long.MinValue
                : -(long)magnitude;
        }

        return new KdlNumber<long>(value, radix, type);
    }

    public static KdlNumber<nint> ParseIntPtr(string input, int sign, int radix, string? type)
    {
        long value = ParseInt64(input, sign, radix, type).Value;
        if (nint.Size == 4)
        {
            if (value < int.MinValue || value > int.MaxValue)
            {
                throw new OverflowException();
            }
        }

        return new KdlNumber<nint>((nint)value, radix, type);
    }

    public static KdlNumber<byte> ParseUInt8(string input, int radix, string? type)
    {
        byte value = Convert.ToByte(input, radix);
        return new KdlNumber<byte>(value, radix, type);
    }

    public static KdlNumber<ushort> ParseUInt16(string input, int radix, string? type)
    {
        ushort value = Convert.ToUInt16(input, radix);
        return new KdlNumber<ushort>(value, radix, type);
    }

    public static KdlNumber<uint> ParseUInt32(string input, int radix, string? type)
    {
        uint value = Convert.ToUInt32(input, radix);
        return new KdlNumber<uint>(value, radix, type);
    }

    public static KdlNumber<ulong> ParseUInt64(string input, int radix, string? type)
    {
        ulong value = Convert.ToUInt64(input, radix);
        return new KdlNumber<ulong>(value, radix, type);
    }

    public static KdlNumber<nuint> ParseUIntPtr(string input, int radix, string? type)
    {
        ulong value = Convert.ToUInt64(input, radix);
        if (nuint.Size == 4 && value > uint.MaxValue)
        {
            throw new OverflowException();
        }

        return new KdlNumber<nuint>((nuint)value, radix, type);
    }

    public static KdlNumber<BigInteger> ParseBigInteger(string input, int sign, int radix, string? type)
    {
        BigInteger value = radix switch
        {
            10 => BigInteger.Parse(input, NumberStyles.None, Culture),
            16 => ParseBigIntegerRadix(input, radix),
            8 => ParseBigIntegerRadix(input, radix),
            2 => ParseBigIntegerRadix(input, radix),
            _ => throw new ArgumentOutOfRangeException(nameof(radix), "Unsupported radix. Expected 2, 8, 10, or 16.")
        };

        if (sign < 0)
        {
            value = -value;
        }

        return new KdlNumber<BigInteger>(value, radix, type);
    }

    private static BigInteger ParseBigIntegerRadix(string input, int radix)
    {
        BigInteger value = BigInteger.Zero;
        foreach (char ch in input)
        {
            int digit;
            if (radix == 16)
            {
                if (!IsHexadecimalDigit(ch))
                {
                    throw new FormatException($"Invalid digit '{ch}' for radix {radix}.");
                }

                digit = HexToNibble(ch);
            }
            else
            {
                if (ch < '0' || ch > '9')
                {
                    throw new FormatException($"Invalid digit '{ch}' for radix {radix}.");
                }

                digit = ch - '0';
            }

            if (digit >= radix)
            {
                throw new FormatException($"Invalid digit '{ch}' for radix {radix}.");
            }

            value *= radix;
            value += digit;
        }

        return value;
    }

    private static ulong ParseUnsignedMagnitude64(string input, int radix)
    {
        return radix switch
        {
            10 => ulong.Parse(input, NumberStyles.None, Culture),
            2 or 8 or 16 => Convert.ToUInt64(input, radix),
            _ => throw new ArgumentOutOfRangeException(nameof(radix), "Unsupported radix. Expected 2, 8, 10, or 16.")
        };
    }

    /// <summary>
    /// Checks whether a character is a valid unicode scalar value.
    /// </summary>
    public static bool IsUnicodeScalarValue(int ucc)
    {
        return (ucc >= 0x0000 && ucc <= 0xd7ff) || (ucc >= 0xe000 && ucc <= 0x10ffff);
    }

    /// <summary>
    /// Checks whether a character is a disallowed KDL code point.
    /// </summary>
    public static bool IsDisallowed(int ucc)
    {
        return (ucc >= 0x00 && ucc <= 0x08)         // various control characters
            || (ucc >= 0x0e && ucc <= 0x1f)         // various control characters
            || (ucc == 0x7f)                        // delete control character (DEL)
            || (ucc >= 0xd800 && ucc <= 0xdfff)     // non unicode scalar values
            || (ucc >= 0x2066 && ucc <= 0x2069)     // directional isolate characters
            || (ucc >= 0x200e && ucc <= 0x200f)     // directional marks
            || (ucc >= 0x202a && ucc <= 0x202e)     // directional control characters
            || (ucc == 0xfeff);                     // zero width no-break space / Byte Order Mark
    }

    /// <summary>
    /// Checks whether a character is a digit between 0 and 9.
    /// </summary>
    public static bool IsDecimalDigit(int ucc)
    {
        return ucc >= '0' && ucc <= '9';
    }

    /// <summary>
    /// Checks whether a character is 0 or 1.
    /// </summary>
    public static bool IsBinaryDigit(int ucc)
    {
        return ucc == '0' || ucc == '1';
    }

    /// <summary>
    /// Checks whether a character is a digit between 0 and 7.
    /// </summary>
    public static bool IsOctalDigit(int ucc)
    {
        return ucc >= '0' && ucc <= '7';
    }

    /// <summary>
    /// Checks whether a character is a digit between 0 and 9, or letters from a-f or A-F.
    /// </summary>
    public static bool IsHexadecimalDigit(int ucc)
    {
        return (ucc >= '0' && ucc <= '9') || (ucc >= 'a' && ucc <= 'f') || (ucc >= 'A' && ucc <= 'F');
    }

    /// <summary>
    /// Checks whether a character is a digit between 0 and 9, an exponent marker ('E' or 'e'), or a plus or minus sign.
    /// </summary>
    public static bool IsFloatingDigit(int ucc)
    {
        return IsDecimalDigit(ucc) || ucc == 'e' || ucc == 'E' || ucc == '+' || ucc == '-' || ucc == '.';
    }

    /// <summary>
    /// Checks whether a character is a digit between 0 and 9 or a plus or minus sign.
    /// </summary>
    public static bool IsValidInitialNumeric(int ucc)
    {
        return (ucc >= '0' && ucc <= '9') || (ucc == '+' || ucc == '-');
    }

    /// <summary>
    /// Checks whether a character can start a bare identifier.
    /// </summary>
    public static bool IsValidIdentifierCharacter(int ucc)
    {
        switch (ucc)
        {
            case '\\':
            case '/':
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case ';':
            case '"':
            case '#':
                return false;
            default:
                break;
        }

        return IsUnicodeScalarValue(ucc)
            && !IsDisallowed(ucc)
            && !IsEqualsSign(ucc)
            && !IsWhitespace(ucc)
            && !IsNewline(ucc);
    }

    /// <summary>
    /// Checks whether a character is valid for a bare identifier.
    /// </summary>
    public static bool IsValidInitialIdentifierCharacter(int ucc)
    {
        return IsValidIdentifierCharacter(ucc) && (ucc < '0' || ucc > '9');
    }

    /// <summary>
    /// Checks whether a character represents empty space.
    /// </summary>
    public static bool IsWhitespace(int ucc)
    {
        switch (ucc)
        {
            case 0x0009: // Character Tabulation
            case 0x0020: // Space
            case 0x00a0: // No-Break Space
            case 0x1680: // Ogham Space Mark
            case 0x2000: // En Quad
            case 0x2001: // Em Quad
            case 0x2002: // En Space
            case 0x2003: // Em Space
            case 0x2004: // Three-Per-Em Space
            case 0x2005: // Four-Per-Em Space
            case 0x2006: // Six-Per-Em Space
            case 0x2007: // Figure Space
            case 0x2008: // Punctuation Space
            case 0x2009: // Thin Space
            case 0x200A: // Hair Space
            case 0x202F: // Narrow No-Break Space
            case 0x205F: // Medium Mathematical Space
            case 0x3000: // Ideographic Space
                return true;
            default:
                return false;
        }
    }

    /// <summary>
    /// Checks whether a character represents a newline.
    /// </summary>
    public static bool IsNewline(int ucc)
    {
        switch (ucc)
        {
            case 0x000d: // Carriage Return
            case 0x000a: // Line Feed
            case 0x0085: // Next Line
            case 0x000b: // Vertical tab
            case 0x000c: // Form Feed
            case 0x2028: // Line Separator
            case 0x2029: // Paragaph Separator
                return true;
            default:
                return false;
        }
    }

    /// <summary>
    /// Checks whether a character represents an equal sign.
    /// </summary>
    public static bool IsEqualsSign(int ucc)
    {
        switch (ucc)
        {
            case 0x003d:    // equals sign (=)
            case 0xfe66:    // small equals sign (﹦)
            case 0xff1d:    // fullwidth equals sign (＝)
            case 0x1f7f0:   // heavy equals sign (🟰)
                return true;
            default:
                return false;
        }
    }

    /// <summary>
    /// Checks whether a character can end a node, which can be a semicolon, a newline, or EOF.
    /// </summary>
    public static bool IsNodeTerminator(int ucc)
    {
        return ucc == EOF || ucc == ';' || IsNewline(ucc);
    }

    /// <summary>
    /// Converts a hexadecimal character to a nibble.
    /// </summary>
    public static int HexToNibble(char c)
    {
        return (c >= '0' && c <= '9') ? c - '0'
            : c >= 'a' && c <= 'f' ? 10 + c - 'a'
            : c >= 'A' && c <= 'F' ? 10 + c - 'A'
            : 0;
    }

    /// <summary>
    /// Writes a KDL string to a TextWriter, quoting and escaping characters as necessary.
    /// </summary>
    public static void WriteString(TextWriter writer, string str, KdlWriteOptions options, bool multiline = false)
    {
        if (str.Length == 0)
        {
            writer.Write("\"\"");
            return;
        }

        int firstIndex = 0;
        int firstCodePoint = ReadCodePoint(str, ref firstIndex);
        bool needsQuotes = multiline || !IsValidInitialIdentifierCharacter(firstCodePoint);

        int codePointIndex = firstIndex;
        while (!needsQuotes && codePointIndex < str.Length)
        {
            int ucc = ReadCodePoint(str, ref codePointIndex);
            needsQuotes = !IsValidIdentifierCharacter(ucc);
        }

        if (!needsQuotes)
        {
            needsQuotes |= str == "inf" || str == "-inf" || str == "nan" || str == "true" || str == "false" || str == "null";
        }

        if (!needsQuotes)
        {
            writer.Write(str);
            return;
        }

        writer.Write('"');
        if (multiline)
        {
            writer.Write('\n');
        }

        for (int i = 0; i < str.Length;)
        {
            int ucc = ReadCodePoint(str, ref i);
            WriteEscaped(writer, ucc, multiline, options);
        }

        if (multiline)
        {
            writer.Write('\n');
        }
        writer.Write('"');
    }

    private static int ReadCodePoint(string str, ref int index)
    {
        char ch = str[index];
        if (char.IsHighSurrogate(ch)
            && index + 1 < str.Length
            && char.IsLowSurrogate(str[index + 1]))
        {
            int codePoint = char.ConvertToUtf32(ch, str[index + 1]);
            index += 2;
            return codePoint;
        }

        ++index;
        return ch;
    }

    public static void WriteEscaped(TextWriter writer, int ucc, bool multiline, KdlWriteOptions options)
    {
        switch (ucc)
        {
            case '\n': writer.Write(multiline || !options.EscapeCommon ? '\n' : "\\n"); break;
            case '\r': writer.Write(multiline || !options.EscapeCommon ? '\r' : "\\r"); break;
            case '\t': writer.Write(options.EscapeCommon ? "\\t" : '\t'); break;
            case '\\': writer.Write("\\\\"); break;
            case '"': writer.Write("\\\""); break;
            case '\b': writer.Write(options.EscapeCommon ? "\\b" : '\b'); break;
            case '\f': writer.Write(options.EscapeCommon ? "\\f" : '\f'); break;
            default:
            {
                if (ucc > MaxUnicode)
                {
                    // TODO: What should we do here?
                }
                else if (IsDisallowed(ucc)
                    || (options.EscapeNonPrintableAscii && ucc < 32)
                    || (options.EscapeNonAscii && ucc > 127))
                {
                    writer.Write($"\\u{{{ucc:x}}}");
                }
                else if (ucc < char.MaxValue)
                {
                    writer.Write((char)ucc);
                }
                else
                {
                    writer.Write(char.ConvertFromUtf32(ucc));
                }
                break;
            }
        }
    }
}
