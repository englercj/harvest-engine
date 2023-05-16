// Copyright Chad Engler

#include "he/core/toml_writer.h"

#include "toml_internal.h"

#include "he/core/ascii.h"
#include "he/core/clock_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/fmt.h"
#include "he/core/string_fmt.h"
#include "he/core/utf8.h"
#include "he/core/vector.h"

#include <cmath>

namespace he
{
    static void WriteEscaped(StringWriter& writer, StringView value)
    {
        for (const char ch : value)
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
                default:
                    if ((ch >= 0x00 && ch <= 0x08) || (ch >= 0x0A && ch <= 0x1F) || ch == 0x7F)
                    {
                        writer.Write("\\u00{}{}", char(48 + (ch / 16)), char((ch % 16 < 10 ? 48 : 55) + (ch % 16)));
                    }
                    else if (ch > 0x7F)
                    {
                        writer.Write("\\x{}{}", char(48 + (ch / 16)), char((ch % 16 < 10 ? 48 : 55) + (ch % 16)));
                    }
                    else
                    {
                        writer.Write(ch);
                    }
            }
        }
    }

    static void WriteEscapedMultiline(StringWriter& writer, StringView value)
    {
        for (char ch : value)
        {
            switch (ch)
            {
                case '\b': writer.Write("\\b"); break;
                case '\t': writer.Write("\\t"); break;
                case '\f': writer.Write("\\f"); break;
                case '\r': writer.Write('\r'); break;
                case '\n': writer.Write('\n'); break;
                case '"': writer.Write("\\\""); break;
                case '\\': writer.Write("\\\\"); break;
                case '\x1B': writer.Write("\\e"); break;
                default:
                    if ((0x00 <= ch && ch <= 0x08) || (0x0A <= ch && ch <= 0x1F) || ch == 0x7F)
                    {
                        writer.Write("\\u00{}{}", char(48 + (ch / 16)), char((ch % 16 < 10 ? 48 : 55) + (ch % 16)));
                    }
                    else
                    {
                        writer.Write(ch);
                    }
            }
        }
    }

    void TomlWriter::Comment(StringView value)
    {
        m_writer.Write('#');
        m_writer.Write(value);
    }

    void TomlWriter::Newline()
    {
        m_writer.Write('\n');
    }

    void TomlWriter::Write(StringView str)
    {
        m_writer.Write(str);
    }

    void TomlWriter::IncreaseIndent()
    {
        m_writer.IncreaseIndent();
    }

    void TomlWriter::DecreaseIndent()
    {
        m_writer.DecreaseIndent();
    }

    void TomlWriter::Bool(bool value)
    {
        ArrayComma();
        m_writer.Write(value ? "true" : "false");
    }

    void TomlWriter::Int(int64_t value, TomlIntFormat format)
    {
        ArrayComma();

        switch (format)
        {
            case TomlIntFormat::Decimal: m_writer.Write("{:d}", value); return;
            case TomlIntFormat::Hex: m_writer.Write("0x{:x}", value); return;
            case TomlIntFormat::Octal: m_writer.Write("0o{:o}", value); return;
            case TomlIntFormat::Binary: m_writer.Write("0b{:b}", value); return;
        }
        HE_VERIFY(false, HE_MSG("Unknown integer format."), HE_KV(format, format));
    }

    void TomlWriter::Uint(uint64_t value, TomlIntFormat format)
    {
        ArrayComma();

        switch (format)
        {
            case TomlIntFormat::Decimal: m_writer.Write("{:d}", value); return;
            case TomlIntFormat::Hex: m_writer.Write("0x{:x}", value); return;
            case TomlIntFormat::Octal: m_writer.Write("0o{:o}", value); return;
            case TomlIntFormat::Binary: m_writer.Write("0b{:b}", value); return;
        }
        HE_VERIFY(false, HE_MSG("Unknown integer format."), HE_KV(format, format));
    }

    void TomlWriter::Float(double value, TomlFloatFormat format, int32_t precision)
    {
        ArrayComma();

        // Handle NaN values
        if (std::isnan(value))
        {
            if (std::signbit(value))
                m_writer.Write("-nan");
            else
                m_writer.Write("nan");
            return;
        }

        // Handle infinite values
        if (!std::isfinite(value))
        {
            if (std::signbit(value))
                m_writer.Write("-inf");
            else
                m_writer.Write("inf");
            return;
        }

        char type = 'g';
        switch (format)
        {
            case TomlFloatFormat::Fixed: type = 'f'; break;
            case TomlFloatFormat::Exponent: type = 'e'; break;
            case TomlFloatFormat::General: type = 'g'; break;
        }

        he::String fmt;
        if (precision > 0)
            FormatTo(fmt, "{{:.{}{}}}", precision, type);
        else
            FormatTo(fmt, "{{:{}}}", type);

        m_writer.Write(FmtRuntime(fmt), value);

        // Ensure that floats like `1.` are written as `1.0`
        if (m_writer.Str().Back() == '.')
            m_writer.Write('0');
    }

    void TomlWriter::String(StringView value, TomlStringFormat format)
    {
        ArrayComma();

        bool multiline = false;

        if (value.Find('\n'))
            multiline = true;

        if (format == TomlStringFormat::Literal)
        {
            // Use multiline literal if the string contains a single quote.
            if (value.Find('\''))
                multiline = true;

            if (multiline)
            {
                m_writer.Write("'''\n");
                m_writer.Write(value);
                m_writer.Write("'''");
            }
            else
            {
                m_writer.Write('\'');
                m_writer.Write(value);
                m_writer.Write('\'');
            }
            return;
        }

        if (multiline)
        {
            m_writer.Write("\"\"\"\n");
            WriteEscapedMultiline(m_writer, value);
            m_writer.Write("\"\"\"");
        }
        else
        {
            m_writer.Write('"');
            WriteEscaped(m_writer, value);
            m_writer.Write('"');
        }
    }

    void TomlWriter::DateTime(SystemTime value, TomlDateTimeFormat format)
    {
        ArrayComma();

        const uint64_t seconds = value.val / Seconds::Ratio;
        const uint64_t nanoseconds = value.val - (seconds * Seconds::Ratio);

        if (format == TomlDateTimeFormat::Utc)
        {
            // RFC3339 UTC date time format
            m_writer.Write("{:%Y-%m-%dT%H:%M:%S}", FmtUtcTime(value));
            if (nanoseconds)
                m_writer.Write(".{}", nanoseconds);
            m_writer.Write('Z');
            return;
        }

        // Ideally we'd be able to use the format: {:%Y-%m-%dT%H:%M:%S%z}
        // Unfortunately, strftime gives us the ISO 8601 format of the local time zone which looks
        // like `-0700` instead of the RFC3339 format of `-07:00`. So we have to do it manually.
        m_writer.Write("{:%Y-%m-%dT%H:%M:%S}", FmtLocalTime(value));

        if (nanoseconds)
            m_writer.Write(".{}", nanoseconds);

        Duration tzOffset = GetLocalTimezoneOffset();
        if (tzOffset.val < 0)
        {
            m_writer.Write('-');
            tzOffset.val = -tzOffset.val;
        }
        else
        {
            m_writer.Write('+');
        }

        const int64_t hours = tzOffset.val / Hours::Ratio;
        const int64_t minutes = (tzOffset.val % Hours::Ratio) / Minutes::Ratio;
        m_writer.Write("{:02}:{:02}", hours, minutes);
    }

    void TomlWriter::Time(Duration value)
    {
        ArrayComma();

        const int64_t hours = value.val / Hours::Ratio;
        const int64_t minutes = (value.val % Hours::Ratio) / Minutes::Ratio;
        const int64_t seconds = (value.val % Minutes::Ratio) / Seconds::Ratio;
        const int64_t nanoseconds = value.val % Seconds::Ratio;
        m_writer.Write("{:02}:{:02}:{:02}.{:09}", hours, minutes, seconds, nanoseconds);
    }

    void TomlWriter::Table(StringView name, bool isArray)
    {
        Table({ &name, 1 }, isArray);
    }

    void TomlWriter::Table(Span<StringView> names, bool isArray)
    {
        if (!HE_VERIFY(m_inlineIndex == 0,
            HE_MSG("Tried to start a table inside an inline table or array."),
            HE_KV(inline_depth, m_inlineIndex)))
        {
            return;
        }

        m_writer.Write('\n');
        m_writer.WriteIndent();
        m_writer.Write('[');
        if (isArray)
            m_writer.Write('[');

        WriteKeyNames(names);

        if (isArray)
            m_writer.Write(']');
        m_writer.Write(']');
    }

    void TomlWriter::Key(StringView name)
    {
        Key({ &name, 1 });
    }

    void TomlWriter::Key(Span<StringView> names)
    {
        InlineTableComma();
        if (m_inlineIndex == 0)
            m_writer.Write('\n');
        m_writer.WriteIndent();
        WriteKeyNames(names);
        m_writer.Write(" = ");
    }

    void TomlWriter::StartInlineTable()
    {
        ArrayComma();
        PushInline(InlineKind::Table);
        m_firstInlineTableKey = true;
        m_writer.Write('{');
    }

    void TomlWriter::EndInlineTable()
    {
        m_writer.Write('}');
        PopInline(InlineKind::Table);
    }

    void TomlWriter::StartArray()
    {
        ArrayComma();
        PushInline(InlineKind::Array);
        m_firstArrayItem = true;
        m_writer.Write('[');
    }

    void TomlWriter::EndArray()
    {
        m_writer.Write(']');
        PopInline(InlineKind::Array);
    }

    void TomlWriter::ArrayComma()
    {
        if (!IsIn(InlineKind::Array))
            return;

        if (!m_firstArrayItem)
            m_writer.Write(", ");

        m_firstArrayItem = false;
    }

    void TomlWriter::InlineTableComma()
    {
        if (!IsIn(InlineKind::Table))
            return;

        if (!m_firstInlineTableKey)
            m_writer.Write(", ");

        m_firstInlineTableKey = false;
    }

    void TomlWriter::WriteKeyName(StringView name)
    {
        if (name.IsEmpty())
        {
            m_writer.Write("\"\"");
            return;
        }

        bool needsQuotes = false;
        const char* begin = name.Begin();
        const char* end = name.End();
        while (begin < end)
        {
            const uint32_t ucc = FromUTF8(begin);

            if (!IsValidTomlKeyCodePoint(ucc))
            {
                needsQuotes = true;
                break;
            }
        }

        if (needsQuotes)
        {
            m_writer.Write('\"');
            WriteEscaped(m_writer, name);
            m_writer.Write('\"');
        }
        else
        {
            m_writer.Write(name);
        }
    }

    void TomlWriter::WriteKeyNames(Span<StringView> names)
    {
        if (names.IsEmpty())
        {
            m_writer.Write("\"\" = ");
            return;
        }

        for (uint32_t i = 0; i < names.Size(); ++i)
        {
            WriteKeyName(names[i]);

            if (i < names.Size() - 1)
                m_writer.Write('.');
        }
    }

    void TomlWriter::PushInline(InlineKind kind)
    {
        if (!HE_VERIFY(m_inlineIndex < MaxInlineDepth))
            return;

        const uint8_t shift = m_inlineIndex % StatesPerByte;
        const uint8_t flag = static_cast<uint8_t>(kind) << shift;
        uint8_t* b = m_inlineStack + (m_inlineIndex / StatesPerByte);
        *b |= flag;
        ++m_inlineIndex;
    }

    void TomlWriter::PopInline(InlineKind kind)
    {
        if (!HE_VERIFY(m_inlineIndex > 0))
            return;

        if (!HE_VERIFY(IsIn(kind)))
            return;

        const uint8_t shift = m_inlineIndex % StatesPerByte;
        const uint8_t flag = static_cast<uint8_t>(InlineKind::All) << shift;
        uint8_t* b = m_inlineStack + (m_inlineIndex / StatesPerByte);
        *b &= ~flag;
        --m_inlineIndex;
    }

    bool TomlWriter::IsIn(InlineKind kind)
    {
        const uint32_t shift = m_inlineIndex % StatesPerByte;
        const uint8_t flag = static_cast<uint8_t>(kind) << shift;
        const uint8_t* b = m_inlineStack + (m_inlineIndex / StatesPerByte);
        return (*b & flag) != 0;
    }

    template <>
    const char* AsString(TomlIntFormat x)
    {
        switch (x)
        {
            case TomlIntFormat::Decimal: return "Decimal";
            case TomlIntFormat::Hex: return "Hex";
            case TomlIntFormat::Octal: return "Octal";
            case TomlIntFormat::Binary: return "Binary";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(TomlFloatFormat x)
    {
        switch (x)
        {
            case TomlFloatFormat::Fixed: return "Fixed";
            case TomlFloatFormat::Exponent: return "Exponent";
            case TomlFloatFormat::General: return "General";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(TomlStringFormat x)
    {
        switch (x)
        {
            case TomlStringFormat::Basic: return "Basic";
            case TomlStringFormat::Literal: return "Literal";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(TomlDateTimeFormat x)
    {
        switch (x)
        {
            case TomlDateTimeFormat::Utc: return "Utc";
            case TomlDateTimeFormat::Local: return "Local";
        }

        return "<unknown>";
    }
}
