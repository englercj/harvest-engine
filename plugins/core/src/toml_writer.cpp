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
    static void WriteCharacter(StringWriter& writer, const char ch)
    {
        const uint8_t ucc = static_cast<uint8_t>(ch);

        if ((0x00 <= ucc && ucc <= 0x08) || (0x0A <= ucc && ucc <= 0x1F) || ucc == 0x7F)
        {
            writer.Write("\\u00{}{}", ToHex(ucc >> 4), ToHex(ucc & 0xf));
        }
        else
        {
            writer.Write(ch);
        }
    }

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
                default: WriteCharacter(writer, ch); break;
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
                default: WriteCharacter(writer, ch); break;
            }
        }
    }
    void TomlWriter::Clear()
    {
        m_firstInlineTableKey = true;
        m_firstArrayItem = true;
        MemZero(m_inlineStack, sizeof(m_inlineStack));
        m_inlineStackSize = 0;
        m_writer.Clear();
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

    void TomlWriter::Int(int64_t value)
    {
        ArrayComma();
        m_writer.Write("{:d}", value);
    }

    void TomlWriter::Uint(uint64_t value, TomlUintFormat format)
    {
        ArrayComma();

        switch (format)
        {
            case TomlUintFormat::Decimal: m_writer.Write("{:d}", value); return;
            case TomlUintFormat::Hex: m_writer.Write("0x{:x}", value); return;
            case TomlUintFormat::Octal: m_writer.Write("0o{:o}", value); return;
            case TomlUintFormat::Binary: m_writer.Write("0b{:b}", value); return;
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
            case TomlFloatFormat::General: type = 'g'; break;
            case TomlFloatFormat::Fixed: type = 'f'; break;
            case TomlFloatFormat::Exponent: type = 'e'; break;
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

        switch (format)
        {
            case TomlDateTimeFormat::OffsetUtc:
            {
                m_writer.Write("{:%Y-%m-%dT%H:%M:%S}", FmtUtcTime(value));

                if (nanoseconds)
                    m_writer.Write(".{:09d}", nanoseconds);

                m_writer.Write('Z');
                break;
            }
            case TomlDateTimeFormat::OffsetLocal:
            {
                m_writer.Write("{:%Y-%m-%dT%H:%M:%S%z}", FmtLocalTime(value));

                // Copy out, and erase, the last 5 characters of the string. These are the
                // ISO 8601 formatted local time zone (e.g. "-0700"), but we want them to be
                // in RFC3339 format (e.g. "-07:00").
                //
                // It is important we copy these out here and don't use the local time zone
                // from the system because the time zone in the output string needs to be based
                // on the date time of the input, not the state of the world when writing it, to
                // ensure proper handling of daylight saving time.
                he::String& str = m_writer.Str();
                constexpr uint32_t TzLen = 5;
                char tzBuf[TzLen];
                MemCopy(tzBuf, str.End() - TzLen, TzLen);
                str.Erase(str.Size() - TzLen, TzLen);

                if (nanoseconds)
                    m_writer.Write(".{:09d}", nanoseconds);

                m_writer.Write(tzBuf[0]);
                m_writer.Write(tzBuf[1]);
                m_writer.Write(tzBuf[2]);
                m_writer.Write(':');
                m_writer.Write(tzBuf[3]);
                m_writer.Write(tzBuf[4]);
                break;
            }
            case TomlDateTimeFormat::Local:
            {
                m_writer.Write("{:%Y-%m-%dT%H:%M:%S}", FmtLocalTime(value));

                if (nanoseconds)
                    m_writer.Write(".{:09d}", nanoseconds);

                break;
            }
        }
    }

    void TomlWriter::Time(Duration value)
    {
        ArrayComma();

        const int64_t hours = value.val / Hours::Ratio;
        const int64_t minutes = (value.val % Hours::Ratio) / Minutes::Ratio;
        const int64_t seconds = (value.val % Minutes::Ratio) / Seconds::Ratio;
        const int64_t nanoseconds = value.val % Seconds::Ratio;

        if (nanoseconds)
            m_writer.Write("{:02}:{:02}:{:02}.{:09}", hours, minutes, seconds, nanoseconds);
        else
            m_writer.Write("{:02}:{:02}:{:02}", hours, minutes, seconds);
    }

    void TomlWriter::Table(StringView name, bool isArray)
    {
        Table({ &name, 1 }, isArray);
    }

    void TomlWriter::Table(Span<StringView> names, bool isArray)
    {
        if (!HE_VERIFY(m_inlineStackSize == 0,
            HE_MSG("Tried to start a table inside an inline table or array."),
            HE_KV(inline_depth, m_inlineStackSize)))
        {
            return;
        }

        if (!m_writer.Str().IsEmpty())
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

        if (m_inlineStackSize == 0 && !m_writer.Str().IsEmpty())
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
        if (!HE_VERIFY(m_inlineStackSize < MaxInlineDepth))
            return;

        const uint32_t index = m_inlineStackSize;
        const uint8_t shift = (index % StatesPerByte) * BitsPerState;
        const uint8_t flag = static_cast<uint8_t>(kind) << shift;
        uint8_t* b = m_inlineStack + (index / StatesPerByte);
        *b |= flag;
        ++m_inlineStackSize;
    }

    void TomlWriter::PopInline(InlineKind kind)
    {
        if (!HE_VERIFY(m_inlineStackSize > 0))
            return;

        if (!HE_VERIFY(IsIn(kind)))
            return;

        const uint32_t index = m_inlineStackSize - 1;
        const uint8_t shift = (index % StatesPerByte) * BitsPerState;
        const uint8_t flag = static_cast<uint8_t>(InlineKind::All) << shift;
        uint8_t* b = m_inlineStack + (index / StatesPerByte);
        *b &= ~flag;
        --m_inlineStackSize;
    }

    bool TomlWriter::IsIn(InlineKind kind)
    {
        if (m_inlineStackSize == 0)
            return false;

        const uint32_t index = m_inlineStackSize - 1;
        const uint8_t shift = (index % StatesPerByte) * BitsPerState;
        const uint8_t flag = static_cast<uint8_t>(kind) << shift;
        const uint8_t* b = m_inlineStack + (index / StatesPerByte);
        return (*b & flag) != 0;
    }

    template <>
    const char* EnumTraits<TomlUintFormat>::ToString(TomlUintFormat x) noexcept
    {
        switch (x)
        {
            case TomlUintFormat::Decimal: return "Decimal";
            case TomlUintFormat::Hex: return "Hex";
            case TomlUintFormat::Octal: return "Octal";
            case TomlUintFormat::Binary: return "Binary";
        }

        return "<unknown>";
    }

    template <>
    const char* EnumTraits<TomlFloatFormat>::ToString(TomlFloatFormat x) noexcept
    {
        switch (x)
        {
            case TomlFloatFormat::General: return "General";
            case TomlFloatFormat::Fixed: return "Fixed";
            case TomlFloatFormat::Exponent: return "Exponent";
        }

        return "<unknown>";
    }

    template <>
    const char* EnumTraits<TomlStringFormat>::ToString(TomlStringFormat x) noexcept
    {
        switch (x)
        {
            case TomlStringFormat::Basic: return "Basic";
            case TomlStringFormat::Literal: return "Literal";
        }

        return "<unknown>";
    }

    template <>
    const char* EnumTraits<TomlDateTimeFormat>::ToString(TomlDateTimeFormat x) noexcept
    {
        switch (x)
        {
            case TomlDateTimeFormat::OffsetUtc: return "OffsetUtc";
            case TomlDateTimeFormat::OffsetLocal: return "OffsetLocal";
            case TomlDateTimeFormat::Local: return "Local";
        }

        return "<unknown>";
    }
}
