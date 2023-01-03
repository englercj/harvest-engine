// Copyright Chad Engler

#include "he/core/toml.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/enum_fmt.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    static void WriteEscaped(StringBuilder& builder, StringView value)
    {
        for (char ch : value)
        {
            switch (ch)
            {
                case '\b': builder.Write("\\b"); break;
                case '\t': builder.Write("\\t"); break;
                case '\n': builder.Write("\\n"); break;
                case '\f': builder.Write("\\f"); break;
                case '\r': builder.Write("\\r"); break;
                case '"': builder.Write("\\\""); break;
                case '\\': builder.Write("\\\\"); break;
                default:
                    if (static_cast<uint32_t>(ch) <= 0x001fu)
                        builder.Write("\\u{:04x}", static_cast<uint32_t>(ch));
                    else
                        builder.Write(ch);
            }
        }
    }

    static void WriteEscapedMultiline(StringBuilder& builder, StringView value)
    {
        for (char ch : value)
        {
            switch (ch)
            {
                case '\b': builder.Write("\\b"); break;
                case '"': builder.Write("\\\""); break;
                case '\\': builder.Write("\\\\"); break;
                // write out whitespace as-is for multiline strings
                case '\t':
                case '\n':
                case '\f':
                case '\r':
                    builder.Write(ch);
                    break;
                default:
                    if (static_cast<uint32_t>(ch) <= 0x001fu)
                        builder.Write("\\u{:04x}", static_cast<uint32_t>(ch));
                    else
                        builder.Write(ch);
            }
        }
    }

    static bool IsIdentifier(char ch)
    {
        return IsAlpha(ch) || IsNumeric(ch) || ch == '_';
    }

    // --------------------------------------------------------------------------------------------
    // Kinds of tokens the lexer will emit while parsing a toml string
    enum class TomlToken : uint8_t
    {
        None,
        Error,

        CloseCurlyBracket,
        CloseSquareBracket,
        Comma,
        Comment,
        Dot,
        Eof,
        Equals,
        Float,
        Identifier,
        Integer,
        OpenCurlyBracket,
        OpenSquareBracket,
        String,

        _Count,
    };

    // Transforms a toml string into a token stream. Does very basic syntactic validation, but
    // no semantic validation.
    class TomlLexer
    {
    public:
        struct Token
        {
            TomlToken kind{ TomlToken::None };
            StringView text{};
            TomlError error{};
            uint32_t line{ 0 };
            uint32_t column{ 0 };
        };

    public:
        TomlError Reset(StringView src)
        {
            m_end = src.End();
            m_cursor = src.Begin();
            m_nextTokenStart = m_cursor;
            m_lineStart = m_cursor;
            m_nextLineStart = m_cursor;
            m_nextError = TomlError::None;
            m_nextToken = TomlToken::None;

            if (src.IsEmpty())
                return TomlError::EmptyFile;

            if (!SkipBOM())
                return TomlError::InvalidBom;

            // hydrate the first token
            NextToken();

            Token firstToken = PeekNextToken();

            if (firstToken.kind == TomlToken::Eof)
                return TomlError::EmptyFile;

            if (firstToken.kind == TomlToken::Error)
                return firstToken.error;

            return TomlError::None;
        }

        Token NextToken()
        {
            Token token = PeekNextToken();

            m_lineStart = m_nextLineStart;
            m_nextTokenStart = m_cursor;
            m_nextError = TomlError::None;
            m_nextToken = LexToken();
            return token;
        }

        Token PeekNextToken()
        {
            HE_ASSERT(m_nextTokenStart >= m_nextLineStart);

            Token token;
            token.kind = m_nextError == TomlError::None ? m_nextToken : TomlToken::Error;
            token.text = { m_nextTokenStart, m_cursor };
            token.line = m_line;
            token.column = static_cast<uint32_t>(m_nextTokenStart - m_nextLineStart) + 1;
            token.error = m_nextError;
            return token;
        }

    private:
        bool CheckEof()
        {
            if (m_cursor >= m_end)
            {
                m_nextError = TomlError::Eof;
                return false;
            }
            return true;
        }

        TomlToken LexToken()
        {
            while (m_cursor < m_end)
            {
                const char ch = *m_cursor++;
                switch (ch)
                {
                    case '\0':
                        --m_cursor;
                        return TomlToken::Eof;
                    case '\n':
                        ++m_line;
                        m_nextLineStart = m_cursor;
                        [[fallthrough]];
                    case ' ':
                    case '\t':
                    case '\v':
                    case '\f':
                    case '\r':
                        ++m_nextTokenStart;
                        break; // continue the loop to search for the next token
                    case '0':
                        return LexLeadingZero();
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '+':
                    case '-':
                        return LexDecNum();
                    case '"':
                        return LexBasicString();
                    case '\'':
                        return LexLiteralString();
                    case '#':
                        return LexComment();
                    case '{':
                        return TomlToken::OpenCurlyBracket;
                    case '}':
                        return TomlToken::CloseCurlyBracket;
                    case '[':
                        return TomlToken::OpenSquareBracket;
                    case ']':
                        return TomlToken::CloseSquareBracket;
                    case '.':
                        return TomlToken::Dot;
                    case ',':
                        return TomlToken::Comma;
                    case '=':
                        return TomlToken::Equals;
                    default:
                        if (IsIdentifier(ch))
                            return LexIdentifier();

                        m_nextError = TomlError::InvalidToken;
                        return TomlToken::Error;
                }
            }
        }

        TomlToken LexLeadingZero()
        {
            if (!CheckEof())
                return TomlToken::Error;

            if (*m_cursor == 'x')
            {
                ++m_cursor;
                return LexHexNum();
            }

            if (*m_cursor == 'o')
            {
                ++m_cursor;
                return LexOctNum();
            }

            if (*m_cursor == 'b')
            {
                ++m_cursor;
                return LexBinNum();
            }

            return LexDecNum();
        }

        TomlToken LexBinNum()
        {
            if (!CheckEof())
                return TomlToken::Error;

            while (m_cursor < m_end && (*m_cursor == '0' || *m_cursor == '1'))
                ++m_cursor;

            return TomlToken::Integer;
        }

        TomlToken LexDecNum()
        {
            if (!CheckEof())
                return TomlToken::Error;

            bool hasDot = false;
            while (m_cursor < m_end && (IsNumeric(*m_cursor) || *m_cursor == '.'))
            {
                if (*m_cursor == '.')
                {
                    // When true there are multiple dots which is not valid
                    if (hasDot)
                    {
                        m_nextError = TomlError::InvalidToken;
                        return TomlToken::Error;
                    }

                    hasDot = true;
                }

                ++m_cursor;
            }

            return hasDot ? TomlToken::Float : TomlToken::Integer;
        }

        TomlToken LexHexNum()
        {
            if (!CheckEof())
                return TomlToken::Error;

            while (m_cursor < m_end && IsHex(*m_cursor))
                ++m_cursor;

            return TomlToken::Integer;
        }

        TomlToken LexOctNum()
        {
            if (!CheckEof())
                return TomlToken::Error;

            while (m_cursor < m_end && *m_cursor >= '0' && *m_cursor <= '7')
                ++m_cursor;

            return TomlToken::Integer;
        }

        TomlToken LexBasicString()
        {
            if (!CheckEof())
                return TomlToken::Error;

            bool isMultiline = false;
            if (*m_cursor == '"')
            {
                ++m_cursor;
                if (m_cursor < m_end && *m_cursor == '"')
                {
                    ++m_cursor;
                    isMultiline = true;
                }
                else
                {
                    // empty string
                    return TomlToken::String;
                }
            }

            uint32_t quoteCount = 0;
            char lastCh = '\0';
            while (true)
            {
                if (!CheckEof())
                    return TomlToken::Error;

                char ch = *m_cursor++;

                if (ch < ' ' && ch != '\t' && (ch != '\n' || isMultiline) && (ch != '\r' || isMultiline))
                {
                    m_nextError = TomlError::InvalidToken;
                    return TomlToken::Error;
                }

                if (ch == '"' && lastCh != '\\')
                {
                    ++quoteCount;
                    if (!isMultiline || quoteCount == 3)
                        return TomlToken::String;
                }
                else
                {
                    quoteCount = 0;
                }

                lastCh = ch;
            }
        }

        TomlToken LexLiteralString()
        {
            if (!CheckEof())
                return TomlToken::Error;

            bool isMultiline = false;
            if (*m_cursor == '\'')
            {
                ++m_cursor;
                if (m_cursor < m_end && *m_cursor == '\'')
                {
                    ++m_cursor;
                    isMultiline = true;
                }
                else
                {
                    // empty string
                    return TomlToken::String;
                }
            }

            uint32_t quoteCount = 0;
            while (true)
            {
                if (!CheckEof())
                    return TomlToken::Error;

                char ch = *m_cursor++;

                if (ch == '\'')
                {
                    ++quoteCount;
                    if (!isMultiline || quoteCount == 3)
                        return TomlToken::String;
                }
                else
                {
                    quoteCount = 0;
                }
            }
        }

        TomlToken LexComment()
        {
            while (*m_cursor && *m_cursor != '\n' && *m_cursor != '\r')
                ++m_cursor;

            return TomlToken::Comment;
        }

        TomlToken LexIdentifier()
        {
            while (*m_cursor && IsIdentifier(*m_cursor))
                ++m_cursor;

            return TomlToken::Identifier;
        }

        bool SkipBOM()
        {
            if (static_cast<uint8_t>(*m_cursor) != 0xef)
                return true;

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbb)
            {
                m_nextError = TomlError::InvalidBom;
                return false;
            }

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbf)
            {
                m_nextError = TomlError::InvalidBom;
                return false;
            }

            ++m_cursor;
            return true;
        }

    private:
        const char* m_end{ nullptr };

        const char* m_cursor{ nullptr };
        const char* m_nextTokenStart{ nullptr };

        const char* m_lineStart{ nullptr };
        const char* m_nextLineStart{ nullptr };

        TomlError m_nextError{ TomlError::None };

        uint32_t m_line{ 1 };
        TomlToken m_nextToken{ TomlToken::None };
    };

    // --------------------------------------------------------------------------------------------
    class TomlParser
    {
    public:
        TomlResult Parse(StringView src, TomlReader::Handler& handler)
        {
            m_handler = &handler;

            if (m_lexer.Reset(src) == TomlError::None)
            {
                ConsumeRootTable();
            }

            return MakeResult();
        }

    private:
        TomlResult MakeResult() const
        {
            if (m_token.error != TomlError::None)
                return TomlResult{ m_token.error, m_token.line, m_token.column };

            return m_result;
        }

        bool SetError(TomlError error)
        {
            m_result = { error, m_token.line, m_token.column };
            return false;
        }

    private:
        [[nodiscard]] bool At(TomlToken expected) const
        {
            return m_token.kind == expected;
        }

        [[nodiscard]] bool AtEnd() const
        {
            return At(TomlToken::Eof);
        }

        [[nodiscard]] bool AtIdentifier(StringView expected) const
        {
            return At(TomlToken::Identifier) && m_token.text == expected;
        }

        [[nodiscard]] bool Expect(TomlToken expected)
        {
            if (!At(expected))
                return SetError(TomlError::InvalidToken);

            return true;
        }

        [[nodiscard]] bool Next()
        {
            m_token = m_lexer.NextToken();
            return m_token.kind != TomlToken::Error;
        }

        [[nodiscard]] bool Next(TomlToken expected)
        {
            return Next() && Expect(expected);
        }

        [[nodiscard]] bool NextDecl()
        {
            do
            {
                if (!Next())
                    return false;
            } while (At(TomlToken::Comment));

            return true;
        }

        [[nodiscard]] bool TryConsume(TomlToken expected)
        {
            if (!At(expected))
                return false;

            NextDecl();
            return true;
        }

        [[nodiscard]] bool Consume(TomlToken expected)
        {
            if (!Expect(expected))
                return false;

            NextDecl();
            return true;
        }

        [[nodiscard]] void ConsumeRootTable()
        {
            if (!m_handler->StartTable())
            {
                SetError(TomlError::Cancelled);
                return;
            }

            uint32_t entryCount = 0;

            while (!AtEnd())
            {
                if (!ConsumeTableEntry())
                    return;

                ++entryCount;

                if (!NextDecl())
                    return;
            }

            if (!m_handler->EndTable(entryCount))
            {
                SetError(TomlError::Cancelled);
                return;
            }
        }

        [[nodiscard]] bool ConsumeTableEntry()
        {
            switch (m_token.kind)
            {
                case TomlToken::String:
                case TomlToken::Identifier:
                {
                    if (!m_handler->Key(m_token.text))
                        return SetError(TomlError::Cancelled);

                    if (!Next(TomlToken::Equals))
                        return false;

                    if (!NextDecl())
                        return false;

                    return ConsumeValue();
                }
                case TomlToken::OpenSquareBracket:
                {
                    // TODO: loop through dotted identifier and compare against the current state
                    // 1. start any tables necessary to get to the current path
                    // 2. start array if this is the first entry
                    // 3. start the table for the end of path, and return
                    return false;
                }
                default:
                    return SetError(TomlError::InvalidToken);
            }
        }

        [[nodiscard]] bool ConsumeValue()
        {
            switch (m_token.kind)
            {
                case TomlToken::Float:
                case TomlToken::Integer:
                case TomlToken::OpenCurlyBracket:
                case TomlToken::OpenSquareBracket:
                case TomlToken::String:
                    if (!m_handler->String(m_token.text))
                        return SetError(TomlError::Cancelled);
                    return true;
            }
        }

    private:
        TomlLexer m_lexer{};
        TomlLexer::Token m_token{};
        TomlResult m_result{};
        TomlReader::Handler* m_handler{ nullptr };
    };

    // --------------------------------------------------------------------------------------------
    TomlResult TomlReader::Read(StringView data, Handler& handler)
    {
        TomlParser parser;
        return parser.Parse(data, handler);
    }

    bool TomlReader::Next()
    {
    }

    bool TomlReader::Next(TomlToken expected)
    {
        Next();
        return Expect(expected);
    }

    // --------------------------------------------------------------------------------------------
    void TomlWriter::Bool(bool value)
    {
        InlineArrayComma();
        m_builder.Write(value ? "true" : "false");
    }

    void TomlWriter::Int(int64_t value, IntFormat format)
    {
        InlineArrayComma();

        switch (format)
        {
            case IntFormat::Decimal: m_builder.Write("{:d}", value); return;
            case IntFormat::Hex: m_builder.Write("0x{:x}", value); return;
            case IntFormat::Octal: m_builder.Write("0o{:o}", value); return;
            case IntFormat::Binary: m_builder.Write("0b{:b}", value); return;
        }
        HE_VERIFY(false, HE_MSG("Unknown integer format."), HE_KV(format, format));
    }

    void TomlWriter::Uint(uint64_t value, IntFormat format)
    {
        InlineArrayComma();

        switch (format)
        {
            case IntFormat::Decimal: m_builder.Write("{:d}", value); return;
            case IntFormat::Hex: m_builder.Write("0x{:x}", value); return;
            case IntFormat::Octal: m_builder.Write("0o{:o}", value); return;
            case IntFormat::Binary: m_builder.Write("0b{:b}", value); return;
        }
        HE_VERIFY(false, HE_MSG("Unknown integer format."), HE_KV(format, format));
    }

    void TomlWriter::Float(double value, uint32_t precision, FloatFormat format)
    {
        InlineArrayComma();

        switch (format)
        {
            case FloatFormat::Fixed: m_builder.Write("{1:.{0}f}", precision, value); return;
            case FloatFormat::Exponent: m_builder.Write("{1:.{0}e}", precision, value); return;
        }
        HE_VERIFY(false, HE_MSG("Unknown float format."), HE_KV(format, format), HE_KV(precision, precision));
    }

    void TomlWriter::String(StringView value, bool multiline, bool literal)
    {
        InlineArrayComma();

        if (multiline)
        {
            if (literal)
            {
                m_builder.Write("'''{}'''", value);
            }
            else
            {
                m_builder.Write("\"\"\"");
                WriteEscapedMultiline(m_builder, value);
                m_builder.Write("\"\"\"");
            }
        }
        else
        {
            if (literal)
            {
                m_builder.Write("'{}'", value);
            }
            else
            {
                m_builder.Write('"');
                WriteEscaped(m_builder, value);
                m_builder.Write('"');
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

        m_builder.WriteIndent();

        if (needsQuotes)
            m_builder.Write("\"{}\" = ", name);
        else
            m_builder.Write("{} = ", name);
    }

    void TomlWriter::StartTable(StringView name)
    {
        if (!m_path.IsEmpty())
            m_path += '.';

        m_path += name;

        m_builder.WriteIndent();

        if (m_arrayCount > m_tableCount)
            m_builder.Write("[[{}]]", m_path);
        else
            m_builder.Write("[{}]", m_path);

        m_builder.IncreaseIndent();
        ++m_tableCount;
    }

    void TomlWriter::EndTable()
    {
        if (!HE_VERIFY(m_tableCount > 0))
            return;

        m_builder.DecreaseIndent();
        --m_tableCount;
    }

    void TomlWriter::StartInlineTable()
    {
        if (m_inlineTableCount == 0)
            m_firstInlineTableKey = true;

        InlineArrayComma();
        m_builder.Write("{ ");
        ++m_inlineTableCount;
    }

    void TomlWriter::EndInlineTable()
    {
        if (!HE_VERIFY(m_inlineTableCount > 0))
            return;

        m_builder.Write(" }");
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
        m_builder.Write("[");
        ++m_inlineArrayCount;
    }

    void TomlWriter::EndInlineArray()
    {
        if (!HE_VERIFY(m_inlineArrayCount > 0))
            return;

        m_builder.Write("]");
        --m_inlineArrayCount;
    }

    void TomlWriter::InlineArrayComma()
    {
        if (m_firstInlineArrayItem)
            return;

        m_builder.Write(", ");
        m_firstInlineArrayItem = false;
    }

    void TomlWriter::InlineTableComma()
    {
        if (m_firstInlineTableKey)
            return;

        m_builder.Write(", ");
        m_firstInlineTableKey = false;
    }
}
