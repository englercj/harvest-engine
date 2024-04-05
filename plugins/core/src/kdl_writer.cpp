// Copyright Chad Engler

#include "he/core/kdl_writer.h"

#include "kdl_internal.h"

#include "he/core/assert.h"
#include "he/core/concepts.h"
#include "he/core/math.h"
#include "he/core/utf8.h"
#include "he/core/utils.h"

namespace he
{
    static void WriteCharacter(StringWriter& writer, const char ch)
    {
        const uint8_t ucc = static_cast<uint8_t>(ch);

        if ((0x00 <= ucc && ucc <= 0x08) || (0x0a <= ucc && ucc <= 0x1f) || ucc == 0x7f)
        {
            writer.Write("\\u00{}{}", ToHex(ucc >> 4), ToHex(ucc & 0xf));
        }
        else
        {
            writer.Write(ch);
        }
    }

    static void WriteEscaped(StringWriter& writer, StringView str)
    {
        for (const char ch : str)
        {
            switch (ch)
            {
                case '\b': writer.Write("\\b"); break;
                case '\t': writer.Write("\\t"); break;
                case '\f': writer.Write("\\f"); break;
                case '\r': writer.Write("\\r"); break;
                case '\n': writer.Write("\\n"); break;
                case '"': writer.Write("\\\""); break;
                case '\\': writer.Write("\\\\"); break;
                case '\x1B': writer.Write("\\e"); break;
                default: WriteCharacter(writer, ch); break;
            }
        }
    }

    static bool WriteString(StringWriter& writer, StringView str)
    {
        if (str.IsEmpty())
        {
            writer.Write("\"\"");
            return true;
        }

        bool needsQuotes = false;
        bool needsMultiline = false;
        const char* begin = str.Begin();
        const char* end = str.End();
        while (begin < end)
        {
            uint32_t ucc = 0;
            const uint32_t len = UTF8Decode(ucc, begin, static_cast<uint32_t>(end - begin));

            if (!HE_VERIFY(len > 0 && len != InvalidCodePoint, HE_MSG("Invalid UTF-8 code point.")))
                return false;

            needsQuotes |= !IsValidKdlIdentifierCodePoint(ucc);
            needsMultiline |= IsKdlNewline(ucc);

            if (needsMultiline)
                break;
        }

        needsQuotes |= str == "inf" || str == "-inf" || str == "nan" || str == "true" || str == "false" || str == "null";

        if (needsMultiline)
        {
            writer.Write("\"\n");
            WriteEscaped(writer, str);
            writer.Write("\n\"");
        }
        else if (needsQuotes)
        {
            writer.Write('\"');
            WriteEscaped(writer, str);
            writer.Write('\"');
        }
        else
        {
            writer.Write(str);
        }
    }

    static void WriteTypeAnnotation(StringWriter& writer, StringView type)
    {
        if (type.IsEmpty())
            return;

        writer.Write('(');
        WriteString(writer, type);
        writer.Write(") ");
    }

    static void WritePropertyName(StringWriter& writer, StringView name)
    {
        writer.Write(' ');
        WriteString(writer, name);
        writer.Write(" =");
    }

    static void WriteValue(StringWriter& writer, bool value)
    {
        writer.Write(value ? "#true" : "#false");
    }

    static void WriteValue(StringWriter& writer, long long value, KdlIntFormat format)
    {
        switch (format)
        {
            default:
            case KdlIntFormat::Decimal:
                writer.Write("{}", value);
                break;
            case KdlIntFormat::Hex:
                writer.Write("0x{:x}", value);
                break;
            case KdlIntFormat::Octal:
                writer.Write("0o{:o}", value);
                break;
            case KdlIntFormat::Binary:
                writer.Write("0b{:b}", value);
                break;
        }
    }

    static void WriteValue(StringWriter& writer, unsigned long long value, KdlIntFormat format)
    {
        switch (format)
        {
            default:
            case KdlIntFormat::Decimal:
                writer.Write("{}", value);
                break;
            case KdlIntFormat::Hex:
                writer.Write("0x{:x}", value);
                break;
            case KdlIntFormat::Octal:
                writer.Write("0o{:o}", value);
                break;
            case KdlIntFormat::Binary:
                writer.Write("0b{:b}", value);
                break;
        }
    }

    static void WriteValue(StringWriter& writer, double value, KdlFloatFormat format, int32_t precision)
    {
        // Handle NaN values
        if (IsNan(value))
        {
            writer.Write("#nan");
            return;
        }

        // Handle infinite values
        if (IsInfinite(value))
        {
            if (HasSignBit(value))
                writer.Write("#-inf");
            else
                writer.Write("#inf");
            return;
        }

        char type = 'g';
        switch (format)
        {
            default:
            case KdlFloatFormat::General: type = 'g'; break;
            case KdlFloatFormat::Fixed: type = 'f'; break;
            case KdlFloatFormat::Exponent: type = 'e'; break;
        }

        String fmt;
        if (precision > 0)
            FormatTo(fmt, "{{:.{}{}}}", precision, type);
        else
            FormatTo(fmt, "{{:{}}}", type);

        writer.Write(FmtRuntime(fmt), value);

        // Ensure that floats like `1.` are written as `1.0`
        if (writer.Str().Back() == '.')
            writer.Write('0');
    }

    static void WriteValue(StringWriter& writer, StringView value, KdlStringFormat format, uint32_t rawDelimiterCount)
    {
        switch (format)
        {
            default:
            case KdlStringFormat::Escaped:
                WriteString(writer, value);
                break;
            case KdlStringFormat::Raw:
                if (!HE_VERIFY(rawDelimiterCount > 0, HE_MSG("Raw string delimiter count must be greater than zero.")))
                {
                    rawDelimiterCount = 1;
                }

                for (uint32_t i = 0; i < rawDelimiterCount; ++i)
                    writer.Write('#');
                writer.Write('\"');
                writer.Write(value);
                writer.Write('\"');
                for (uint32_t i = 0; i < rawDelimiterCount; ++i)
                    writer.Write('#');
                break;
        }
    }

    static void WriteValue(StringWriter& writer, nullptr_t)
    {
        writer.Write("#null");
    }

    void KdlWriter::Clear()
    {
        m_writer.Clear();
    }

    void KdlWriter::Comment(StringView value)
    {
        m_writer.Write("// ");
        m_writer.Write(value);
    }

    void KdlWriter::StartComment()
    {
        m_writer.Write("/* ");
    }

    void KdlWriter::EndComment()
    {
        m_writer.Write("*/");
    }

    void KdlWriter::Node(StringView name, StringView type)
    {
        if (!m_startOfLine || m_inNode)
            m_writer.Write('\n');

        WriteTypeAnnotation(m_writer, type);
        WriteString(m_writer, name);
        m_startOfLine = true;
        m_inNode = true;
    }

    void KdlWriter::StartNodeChildren()
    {
        if (!HE_VERIFY(m_inNode, HE_MSG("Cannot start node children outside of a node.")))
            return;

        m_writer.Write(" {\n");
        m_writer.IncreaseIndent();
        m_startOfLine = true;
        m_inNode = false;
        ++m_nodeDepth;
    }

    void KdlWriter::EndNodeChildren()
    {
        if (!HE_VERIFY(m_nodeDepth > 0))
            return;

        if (m_inNode)
            m_writer.Write('\n');

        m_writer.DecreaseIndent();
        m_writer.Write("}\n");
        m_startOfLine = true;
        m_inNode = false;
        --m_nodeDepth;
    }

    template <typename T, typename... Args>
    void WriteArg(StringWriter& writer, T value, StringView type, Args&&... args)
    {
        if (!HE_VERIFY(m_inNode, HE_MSG("Cannot write argument outside of a node.")))
            return;

        writer.Write(' ');
        WriteTypeAnnotation(writer, type);
        WriteValue(writer, value, Forward<Args>(args)...);
    }

    void KdlWriter::Argument(bool value, StringView type) { WriteArg(m_writer, value, type); }
    void KdlWriter::Argument(double value, StringView type, KdlFloatFormat format, int32_t precision) { WriteArg(m_writer, value, type, format, precision); }
    void KdlWriter::Argument(StringView value, StringView type, KdlStringFormat format, uint32_t rawDelimiterCount) { WriteArg(m_writer, value, type, format, rawDelimiterCount); }
    void KdlWriter::Argument(nullptr_t value, StringView type) { WriteArg(m_writer, value, type); }

    void KdlWriter::IntArg(long long value, StringView type, KdlIntFormat format) { WriteArg(m_writer, value, type, format); }
    void KdlWriter::UintArg(unsigned long long value, StringView type, KdlIntFormat format) { WriteArg(m_writer, value, type, format); }

    template <typename T, typename... Args>
    void WriteProp(StringWriter& writer, StringView name, T value, StringView type, Args&&... args)
    {
        if (!HE_VERIFY(m_inNode, HE_MSG("Cannot write property outside of a node.")))
            return;

        WritePropertyName(m_writer, name);
        WriteTypeAnnotation(m_writer, type);
        WriteValue(m_writer, value);
    }

    void KdlWriter::Property(StringView name, bool value, StringView type) { WriteProp(m_writer, name, value, type); }
    void KdlWriter::Property(StringView name, double value, StringView type, KdlFloatFormat format, int32_t precision) { WriteProp(m_writer, name, value, type, format, precision); }
    void KdlWriter::Property(StringView name, StringView value, StringView type, KdlStringFormat format, uint32_t rawDelimiterCount) { WriteProp(m_writer, name, value, type, format, rawDelimiterCount); }
    void KdlWriter::Property(StringView name, nullptr_t value, StringView type) { WriteProp(m_writer, name, value, type); }

    void KdlWriter::IntProp(StringView name, long long value, StringView type, KdlIntFormat format) { WriteProp(m_writer, name, value, type, format); }
    void KdlWriter::UintProp(StringView name, unsigned long long value, StringView type, KdlIntFormat format) { WriteProp(m_writer, name, value, type, format); }
}
