// Copyright Chad Engler

#include "he/core/kdl_writer.h"

#include "kdl_internal.h"

#include "he/core/assert.h"
#include "he/core/concepts.h"
#include "he/core/enum_ops.h"
#include "he/core/fmt.h"
#include "he/core/math.h"
#include "he/core/string.h"
#include "he/core/type_traits.h"
#include "he/core/utf8.h"

namespace he
{
    static void WriteEscaped(StringWriter& writer, StringView str, bool multiline)
    {
        for (const uint32_t ucc : Utf8Splitter(str))
        {
            switch (ucc)
            {
                case '\n': multiline ? writer.Write('\n') : writer.Write("\\n"); break;
                case '\r': multiline ? writer.Write('\r') : writer.Write("\\r"); break;
                case '\t': writer.Write("\\t"); break;
                case '\\': writer.Write("\\\\"); break;
                case '"': writer.Write("\\\""); break;
                case '\b': writer.Write("\\b"); break;
                case '\f': writer.Write("\\f"); break;
                default:
                {
                    if (ucc > 0x10ffff) [[unlikely]]
                    {
                        // TODO: This is not a valid codepoint. What should we do here?
                        // Right now we just silently ignore it.
                    }
                    else if (IsDisallowedKdlCodePoint(ucc))
                    {
                        writer.Write("\\u{{{:x}}}", ucc);
                    }
                    else
                    {
                        UTF8Encode(writer.Str(), ucc);
                    }
                    break;
                }
            }
        }
    }

    static bool WriteString(StringWriter& writer, StringView str, bool multiline)
    {
        bool first = true;
        bool needsQuotes = multiline || str.IsEmpty();
        const char* begin = str.Begin();
        const char* end = str.End();
        while (!needsQuotes && begin < end)
        {
            uint32_t ucc = 0;
            const uint32_t len = UTF8Decode(ucc, begin, static_cast<uint32_t>(end - begin));

            if (!HE_VERIFY(len > 0 && len != InvalidCodePoint, HE_MSG("Invalid UTF-8 code point.")))
            {
                return false;
            }

            if (first)
            {
                first = false;
                needsQuotes = !IsValidKdlIdentifierStartCodePoint(ucc);
            }
            else
            {
                needsQuotes = !IsValidKdlIdentifierCodePoint(ucc);
            }

            if (needsQuotes)
            {
                break;
            }

            begin += len;
        }

        if (!needsQuotes)
        {
            needsQuotes |= str == "inf" || str == "-inf" || str == "nan" || str == "true" || str == "false" || str == "null";
        }

        if (needsQuotes)
        {
            writer.Write('\"');
            if (multiline)
            {
                writer.Write("\"\"\n");
            }
            WriteEscaped(writer, str, multiline);
            if (multiline)
            {
                writer.Write("\n\"\"");
            }
            writer.Write('\"');
        }
        else
        {
            HE_ASSERT(!multiline);
            writer.Write(str);
        }

        return true;
    }

    static void WriteTypeAnnotation(StringWriter& writer, StringView type)
    {
        writer.Write('(');
        WriteString(writer, type, false);
        writer.Write(')');
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
                writer.Write("{:#x}", value);
                break;
            case KdlIntFormat::Octal:
            {
                // Fmt uses "0" as the prefix for octal when formatted with "#".
                // Since we use "0o" as the prefix, we need to do it manually.
                const bool isSigned = value < 0;
                writer.Write("{}0o{:o}", isSigned ? "-" : "", isSigned ? -value : value);
                break;
            }
            case KdlIntFormat::Binary:
                writer.Write("{:#b}", value);
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
                writer.Write("{:#x}", value);
                break;
            case KdlIntFormat::Octal:
                writer.Write("0o{:o}", value);
                break;
            case KdlIntFormat::Binary:
                writer.Write("{:#b}", value);
                break;
        }
    }

    static String GetFloatFormatStr(char type, int32_t precision)
    {
        String fmt;
        if (precision > 0)
        {
            FormatTo(fmt, "{{:#.{}{}}}", precision, type);
        }
        else
        {
            FormatTo(fmt, "{{:#{}}}", type);
        }
        return fmt;
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
            {
                writer.Write("#-inf");
            }
            else
            {
                writer.Write("#inf");
            }
            return;
        }

        switch (format)
        {
            case KdlFloatFormat::Default:
            {
                writer.Write("{:#D}", value);
                return;
            }
            case KdlFloatFormat::General:
            {
                const String fmt = GetFloatFormatStr('G', precision);
                writer.Write(FmtRuntime(fmt), value);
                return;
            }
            case KdlFloatFormat::Fixed:
            {
                const String fmt = GetFloatFormatStr('F', precision);
                writer.Write(FmtRuntime(fmt), value);
                return;
            }
            case KdlFloatFormat::Exponent:
            {
                const String fmt = GetFloatFormatStr('E', precision);
                writer.Write(FmtRuntime(fmt), value);
                return;
            }
        }

        HE_VERIFY(false, HE_MSG("Invalid KdlFloatFormat."), HE_VAL(format), HE_VAL(precision), HE_VAL(value));
        writer.Write("{:#}", value);
    }

    static void WriteValue(StringWriter& writer, StringView value, bool multiline, uint32_t rawDelimiterCount)
    {
        if (rawDelimiterCount == 0)
        {
            WriteString(writer, value, multiline);
            return;
        }

        for (uint32_t i = 0; i < rawDelimiterCount; ++i)
        {
            writer.Write('#');
        }

        writer.Write('\"');

        if (multiline)
        {
            writer.Write('\n');
        }

        writer.Write(value);

        if (multiline)
        {
            writer.Write('\n');
        }

        writer.Write('\"');

        for (uint32_t i = 0; i < rawDelimiterCount; ++i)
        {
            writer.Write('#');
        }
    }

    static void WriteValue(StringWriter& writer, const char* value, bool multiline, uint32_t rawDelimiterCount)
    {
        WriteValue(writer, StringView(value), multiline, rawDelimiterCount);
    }

    static void WriteValue(StringWriter& writer, nullptr_t)
    {
        writer.Write("#null");
    }

    void KdlWriter::Clear()
    {
        m_writer.Clear();
        m_nodeDepth = 0;
        m_startOfLine = true;
        m_inNode = false;
    }

    void KdlWriter::Comment(StringView value)
    {
        m_writer.Write("// ");
        m_writer.Write(value);
        m_startOfLine = false;
    }

    void KdlWriter::StartComment()
    {
        m_writer.Write("/* ");
        m_startOfLine = false;
    }

    void KdlWriter::EndComment()
    {
        m_writer.Write("*/");
        m_startOfLine = false;
    }

    void KdlWriter::Node(StringView name, const StringView* type)
    {
        if (!m_startOfLine || m_inNode)
        {
            m_writer.Write('\n');
        }

        m_writer.WriteIndent();
        if (type)
        {
            WriteTypeAnnotation(m_writer, *type);
        }
        WriteString(m_writer, name, false);
        m_startOfLine = false;
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
        {
            m_writer.Write('\n');
        }

        m_writer.DecreaseIndent();
        m_writer.WriteIndent();
        m_writer.Write("}\n");
        m_startOfLine = true;
        m_inNode = false;
        --m_nodeDepth;
    }

    template <typename T, typename... Args>
    void KdlWriter::WriteArgOrProp(const StringView* name, T value, const StringView* type, Args&&... args)
    {
        if (!HE_VERIFY(m_inNode, HE_MSG("Cannot write argument or property outside of a node.")))
            return;

        m_startOfLine = false;

        m_writer.Write(' ');

        if (name)
        {
            WriteString(m_writer, *name, false);
            m_writer.Write('=');
        }

        if (type)
        {
            WriteTypeAnnotation(m_writer, *type);
        }

        if constexpr (IsIntegral<T> && IsSigned<T>)
        {
            WriteValue(m_writer, static_cast<int64_t>(value), Forward<Args>(args)...);
        }
        else if constexpr (IsIntegral<T> && IsUnsigned<T>)
        {
            WriteValue(m_writer, static_cast<uint64_t>(value), Forward<Args>(args)...);
        }
        else
        {
            WriteValue(m_writer, value, Forward<Args>(args)...);
        }
    }


    template <> void KdlWriter::Argument<signed char>(signed char value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }
    template <> void KdlWriter::Argument<short>(short value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }
    template <> void KdlWriter::Argument<int>(int value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }
    template <> void KdlWriter::Argument<long>(long value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }
    template <> void KdlWriter::Argument<long long>(long long value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }

    template <> void KdlWriter::Argument<unsigned char>(unsigned char value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }
    template <> void KdlWriter::Argument<unsigned short>(unsigned short value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }
    template <> void KdlWriter::Argument<unsigned int>(unsigned int value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }
    template <> void KdlWriter::Argument<unsigned long>(unsigned long value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }
    template <> void KdlWriter::Argument<unsigned long long>(unsigned long long value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(nullptr, value, type, format); }

    void KdlWriter::Argument(bool value, const StringView* type) { WriteArgOrProp(nullptr, value, type); }
    void KdlWriter::Argument(double value, const StringView* type, KdlFloatFormat format, int32_t precision) { WriteArgOrProp(nullptr, value, type, format, precision); }
    void KdlWriter::Argument(StringView value, const StringView* type, bool multiline, uint32_t rawDelimiterCount) { WriteArgOrProp(nullptr, value, type, multiline, rawDelimiterCount); }
    void KdlWriter::Argument(const char* value, const StringView* type, bool multiline, uint32_t rawDelimiterCount) { WriteArgOrProp(nullptr, value, type, multiline, rawDelimiterCount); }
    void KdlWriter::Argument(nullptr_t value, const StringView* type) { WriteArgOrProp(nullptr, value, type); }

    template <> void KdlWriter::Property<signed char>(StringView name, signed char value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }
    template <> void KdlWriter::Property<short>(StringView name, short value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }
    template <> void KdlWriter::Property<int>(StringView name, int value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }
    template <> void KdlWriter::Property<long>(StringView name, long value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }
    template <> void KdlWriter::Property<long long>(StringView name, long long value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }

    template <> void KdlWriter::Property<unsigned char>(StringView name, unsigned char value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }
    template <> void KdlWriter::Property<unsigned short>(StringView name, unsigned short value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }
    template <> void KdlWriter::Property<unsigned int>(StringView name, unsigned int value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }
    template <> void KdlWriter::Property<unsigned long>(StringView name, unsigned long value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }
    template <> void KdlWriter::Property<unsigned long long>(StringView name, unsigned long long value, const StringView* type, KdlIntFormat format) { return WriteArgOrProp(&name, value, type, format); }

    void KdlWriter::Property(StringView name, bool value, const StringView* type) { WriteArgOrProp(&name, value, type); }
    void KdlWriter::Property(StringView name, double value, const StringView* type, KdlFloatFormat format, int32_t precision) { WriteArgOrProp(&name, value, type, format, precision); }
    void KdlWriter::Property(StringView name, StringView value, const StringView* type, bool multiline, uint32_t rawDelimiterCount) { WriteArgOrProp(&name, value, type, multiline, rawDelimiterCount); }
    void KdlWriter::Property(StringView name, const char* value, const StringView* type, bool multiline, uint32_t rawDelimiterCount) { WriteArgOrProp(&name, value, type, multiline, rawDelimiterCount); }
    void KdlWriter::Property(StringView name, nullptr_t value, const StringView* type) { WriteArgOrProp(&name, value, type); }

    template <>
    const char* EnumTraits<KdlIntFormat>::ToString(KdlIntFormat x) noexcept
    {
        switch (x)
        {
            case KdlIntFormat::Decimal: return "Decimal";
            case KdlIntFormat::Hex: return "Hex";
            case KdlIntFormat::Octal: return "Octal";
            case KdlIntFormat::Binary: return "Binary";
        }

        return "<unknown>";
    }

    template <>
    const char* EnumTraits<KdlFloatFormat>::ToString(KdlFloatFormat x) noexcept
    {
        switch (x)
        {
            case KdlFloatFormat::Default: return "Default";
            case KdlFloatFormat::General: return "General";
            case KdlFloatFormat::Fixed: return "Fixed";
            case KdlFloatFormat::Exponent: return "Exponent";
        }

        return "<unknown>";
    }
}
