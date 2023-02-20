// Copyright Chad Engler

#include "he/core/toml_writer.h"

#include "he/core/ascii.h"
#include "he/core/fmt.h"
#include "he/core/string_fmt.h"
#include "he/core/vector.h"

namespace he
{
    static void WriteEscaped(StringWriter& writer, StringView value)
    {
        for (char ch : value)
        {
            switch (ch)
            {
                case '\b': writer.Write("\\b"); break;
                case '\t': writer.Write("\\t"); break;
                case '\n': writer.Write("\\n"); break;
                case '\f': writer.Write("\\f"); break;
                case '\r': writer.Write("\\r"); break;
                case '"': writer.Write("\\\""); break;
                case '\\': writer.Write("\\\\"); break;
                default:
                    if (static_cast<uint32_t>(ch) <= 0x001fu)
                        writer.Write("\\u{:04x}", static_cast<uint32_t>(ch));
                    else
                        writer.Write(ch);
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
                case '"': writer.Write("\\\""); break;
                case '\\': writer.Write("\\\\"); break;
                // write out whitespace as-is for multiline strings
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                    writer.Write(ch);
                    break;
                default:
                    if (static_cast<uint32_t>(ch) <= 0x001fu)
                        writer.Write("\\u{:04x}", static_cast<uint32_t>(ch));
                    else
                        writer.Write(ch);
            }
        }
    }

    void TomlWriter::Bool(bool value)
    {
        InlineArrayComma();
        m_writer.Write(value ? "true" : "false");
    }

    void TomlWriter::Int(int64_t value, TomlIntFormat format)
    {
        InlineArrayComma();

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
        InlineArrayComma();

        switch (format)
        {
            case TomlIntFormat::Decimal: m_writer.Write("{:d}", value); return;
            case TomlIntFormat::Hex: m_writer.Write("0x{:x}", value); return;
            case TomlIntFormat::Octal: m_writer.Write("0o{:o}", value); return;
            case TomlIntFormat::Binary: m_writer.Write("0b{:b}", value); return;
        }
        HE_VERIFY(false, HE_MSG("Unknown integer format."), HE_KV(format, format));
    }

    void TomlWriter::Float(double value, int32_t precision, TomlFloatFormat format)
    {
        InlineArrayComma();

        char type = 'g';
        switch (format)
        {
            case TomlFloatFormat::Fixed: type = 'f'; return;
            case TomlFloatFormat::Exponent: type = 'e'; return;
            case TomlFloatFormat::General: type = 'g'; return;
        }

        he::String fmt;
        if (precision > 0)
            FormatTo(fmt, "{{:.{}{}}}", precision, type);
        else
            FormatTo(fmt, "{{:{}}}", type);

        m_writer.Write(FmtRuntime(fmt), value);
    }

    void TomlWriter::String(StringView value, bool multiline, bool literal)
    {
        InlineArrayComma();

        if (multiline)
        {
            if (literal)
            {
                m_writer.Write("'''{}'''", value);
            }
            else
            {
                m_writer.Write("\"\"\"");
                WriteEscapedMultiline(m_writer, value);
                m_writer.Write("\"\"\"");
            }
        }
        else
        {
            if (literal)
            {
                m_writer.Write("'{}'", value);
            }
            else
            {
                m_writer.Write('"');
                WriteEscaped(m_writer, value);
                m_writer.Write('"');
            }
        }
    }

    void TomlWriter::Key(StringView name)
    {
        InlineTableComma();

        bool needsQuotes = false;
        for (char ch : name)
        {
            if (!IsAlphaNum(ch) && ch != '_' && ch != '-')
            {
                needsQuotes = true;
                break;
            }
        }

        m_writer.WriteIndent();

        if (needsQuotes)
            m_writer.Write("\"{}\" = ", name);
        else
            m_writer.Write("{} = ", name);
    }

    void TomlWriter::StartTable(StringView name)
    {
        if (!m_path.IsEmpty())
            m_path += '.';

        m_path += name;

        m_writer.WriteIndent();

        if (m_arrayCount > m_tableCount)
            m_writer.Write("[[{}]]", m_path);
        else
            m_writer.Write("[{}]", m_path);

        m_writer.IncreaseIndent();
        ++m_tableCount;
    }

    void TomlWriter::EndTable()
    {
        if (!HE_VERIFY(m_tableCount > 0))
            return;

        m_writer.DecreaseIndent();
        --m_tableCount;
    }

    void TomlWriter::StartInlineTable()
    {
        if (m_inlineTableCount == 0)
            m_firstInlineTableKey = true;

        InlineArrayComma();
        m_writer.Write("{ ");
        ++m_inlineTableCount;
    }

    void TomlWriter::EndInlineTable()
    {
        if (!HE_VERIFY(m_inlineTableCount > 0))
            return;

        m_writer.Write(" }");
        --m_inlineTableCount;
    }

    void TomlWriter::StartArray()
    {
        ++m_arrayCount;
    }

    void TomlWriter::EndArray()
    {
        if (!HE_VERIFY(m_arrayCount > 0))
            return;

        --m_arrayCount;
    }

    void TomlWriter::StartInlineArray()
    {
        if (m_inlineArrayCount == 0)
            m_firstInlineArrayItem = true;

        InlineArrayComma();
        m_writer.Write("[");
        ++m_inlineArrayCount;
    }

    void TomlWriter::EndInlineArray()
    {
        if (!HE_VERIFY(m_inlineArrayCount > 0))
            return;

        m_writer.Write("]");
        --m_inlineArrayCount;
    }

    void TomlWriter::InlineArrayComma()
    {
        if (m_firstInlineArrayItem)
            return;

        m_writer.Write(", ");
        m_firstInlineArrayItem = false;
    }

    void TomlWriter::InlineTableComma()
    {
        if (m_firstInlineTableKey)
            return;

        m_writer.Write(", ");
        m_firstInlineTableKey = false;
    }
}
