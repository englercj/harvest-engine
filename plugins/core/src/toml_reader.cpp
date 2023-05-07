// Copyright Chad Engler

// TODO: Unreleased features as of v1.0.0 that are not yet supported:
// - Clarify Unicode and UTF-8 references.
// - Relax comment parsing; most control characters are again permitted.
// - Clarify where and how dotted keys define tables.
// - Add new \e shorthand for the escape character.
// - Add \x00 notation to basic strings.
// - Seconds in Date-Time and Time values are now optional.
// - Allow non-English scripts in unquoted (bare) keys
// - Clarify newline normalization in multi-line literal strings.

#include "he/core/toml_reader.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/string_fmt.h"
#include "he/core/vector.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    static bool IsIdentifier(char ch)
    {
        return IsAlpha(ch) || IsNumeric(ch) || ch == '_';
    }

    static bool IsControlChar(char ch)
    {
        return (ch >= '\x00' && ch <= '\x08')
            || (ch >= '\x0a' && ch <= '\x1F')
            || ch == '\x7F';
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
            TomlReadError error{};
            uint32_t line{ 0 };
            uint32_t column{ 0 };
        };

    public:
        TomlReadError Reset(StringView src)
        {
            m_end = src.End();
            m_cursor = src.Begin();
            m_nextTokenStart = m_cursor;
            m_lineStart = m_cursor;
            m_nextLineStart = m_cursor;
            m_nextError = TomlReadError::None;
            m_nextToken = TomlToken::None;

            if (src.IsEmpty())
                return TomlReadError::EmptyFile;

            if (!SkipBOM())
                return TomlReadError::InvalidBom;

            // hydrate the first token
            NextToken();

            Token firstToken = PeekNextToken();

            if (firstToken.kind == TomlToken::Eof)
                return TomlReadError::EmptyFile;

            if (firstToken.kind == TomlToken::Error)
                return firstToken.error;

            return TomlReadError::None;
        }

        Token NextToken()
        {
            Token token = PeekNextToken();

            m_lineStart = m_nextLineStart;
            m_nextTokenStart = m_cursor;
            m_nextError = TomlReadError::None;
            m_nextToken = LexToken();
            return token;
        }

        Token PeekNextToken()
        {
            HE_ASSERT(m_nextTokenStart >= m_nextLineStart);

            Token token;
            token.kind = m_nextError == TomlReadError::None ? m_nextToken : TomlToken::Error;
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
                m_nextError = TomlReadError::Eof;
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

                        m_nextError = TomlReadError::InvalidToken;
                        return TomlToken::Error;
                }
            }

            return TomlToken::Eof;
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
                        m_nextError = TomlReadError::InvalidToken;
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
                    m_nextError = TomlReadError::InvalidToken;
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
            {
                if (IsControlChar(*m_cursor))
                {
                    m_nextError = TomlReadError::InvalidToken;
                    return TomlToken::Error;
                }
                ++m_cursor;
            }

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
                m_nextError = TomlReadError::InvalidBom;
                return false;
            }

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbf)
            {
                m_nextError = TomlReadError::InvalidBom;
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

        TomlReadError m_nextError{ TomlReadError::None };

        uint32_t m_line{ 1 };
        TomlToken m_nextToken{ TomlToken::None };
    };

    // --------------------------------------------------------------------------------------------
    class TomlParser
    {
    public:
        TomlReadResult Parse(StringView src, TomlReader::Handler& handler)
        {
            m_handler = &handler;

            if (m_lexer.Reset(src) == TomlReadError::None)
            {
                ConsumeRootTable();
            }

            return MakeResult();
        }

    private:
        TomlReadResult MakeResult() const
        {
            if (m_token.error != TomlReadError::None)
                return TomlReadResult{ m_token.error, m_token.line, m_token.column };

            return m_result;
        }

        bool SetError(TomlReadError error)
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
                return SetError(TomlReadError::InvalidToken);

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
                if (At(TomlToken::Comment))
                {
                    m_handler->Comment(m_token.text);
                }

                if (!Next())
                    return false;
            } while (At(TomlToken::Comment));

            return true;
        }

        [[nodiscard]] bool TryConsume(TomlToken expected)
        {
            if (!At(expected))
                return false;

            return NextDecl();
        }

        [[nodiscard]] bool Consume(TomlToken expected)
        {
            if (!Expect(expected))
                return false;

            return NextDecl();
        }

        [[nodiscard]] bool ConsumeKeyPath()
        {
            m_pathBuffer.Clear();
            do
            {
                if (At(TomlToken::Identifier) || At(TomlToken::String))
                    m_pathBuffer.PushBack(m_token.text);
                else
                    return SetError(TomlReadError::InvalidToken);

            } while (TryConsume(TomlToken::Dot));

            return true;
        }

        bool ConsumeRootTable()
        {
            m_handler->StartDocument();

            while (!AtEnd())
            {
                if (!ConsumeTableEntry())
                    return false;
            }

            m_handler->EndDocument();
            return true;
        }

        [[nodiscard]] bool ConsumeTableEntry()
        {
            switch (m_token.kind)
            {
                case TomlToken::Comment:
                    return NextDecl();
                case TomlToken::String:
                case TomlToken::Identifier:
                {
                    if (!ConsumeKeyPath())
                        return false;

                    if (!m_handler->Key(m_pathBuffer))
                        return SetError(TomlReadError::Cancelled);

                    if (!Consume(TomlToken::Equals))
                        return false;

                    return ConsumeValue();
                }
                case TomlToken::OpenSquareBracket:
                {
                    return ConsumeTable();
                }
                default:
                    return SetError(TomlReadError::InvalidToken);
            }
        }

        [[nodiscard]] bool ConsumeTable()
        {
            if (!Consume(TomlToken::OpenSquareBracket))
                return false;

            const bool isArray = TryConsume(TomlToken::OpenSquareBracket);

            if (!ConsumeKeyPath())
                return false;

            if (!m_handler->StartTable(m_pathBuffer, isArray))
                return SetError(TomlReadError::Cancelled);

            uint32_t keyCount = 0;
            while (!AtEnd() && !At(TomlToken::OpenSquareBracket))
            {
                if (!ConsumeTableEntry())
                    return false;
                ++keyCount;
            }

            if (!m_handler->EndTable(keyCount))
                return SetError(TomlReadError::Cancelled);

            return true;
        }

        [[nodiscard]] bool ConsumeInlineTable()
        {
            if (!Consume(TomlToken::OpenCurlyBracket))
                return false;

            if (!m_handler->StartTable({}, false))
                return SetError(TomlReadError::Cancelled);

            uint32_t keyCount = 0;
            do
            {
                if (TryConsume(TomlToken::CloseCurlyBracket))
                    break;

                if (!ConsumeInlineTableEntry())
                    return false;

                ++keyCount;
            } while (TryConsume(TomlToken::Comma));

            if (!m_handler->EndTable(keyCount))
                return SetError(TomlReadError::Cancelled);

            return true;
        }

        [[nodiscard]] bool ConsumeInlineTableEntry()
        {
            switch (m_token.kind)
            {
                case TomlToken::String:
                case TomlToken::Identifier:
                {
                    if (!ConsumeKeyPath())
                        return false;

                    if (!m_handler->Key(m_pathBuffer))
                        return SetError(TomlReadError::Cancelled);

                    if (!Consume(TomlToken::Equals))
                        return false;

                    return ConsumeValue();
                }
                default:
                    return SetError(TomlReadError::InvalidToken);
            }
        }

        [[nodiscard]] bool ConsumeInlineArray()
        {
            if (!Consume(TomlToken::OpenSquareBracket))
                return false;

            if (!m_handler->StartArray())
                return SetError(TomlReadError::Cancelled);

            uint32_t count = 0;
            do
            {
                if (TryConsume(TomlToken::CloseSquareBracket))
                    break;

                if (!ConsumeValue())
                    return false;

                ++count;
            } while (!AtEnd() && TryConsume(TomlToken::Comma));

            if (!m_handler->EndArray(count))
                return SetError(TomlReadError::Cancelled);

            return true;
        }

        [[nodiscard]] bool ConsumeValue()
        {
            switch (m_token.kind)
            {
                case TomlToken::Float:
                {
                    const double value = m_token.text.ToFloat<double>();

                    if (!m_handler->Float(value))
                        return SetError(TomlReadError::Cancelled);

                    return NextDecl();
                }
                case TomlToken::Integer:
                {
                    if (m_token.text[0] == '-')
                    {
                        const int64_t value = m_token.text.ToInteger<int64_t>();
                        if (!m_handler->Int(value))
                            return SetError(TomlReadError::Cancelled);
                    }
                    else
                    {
                        const char* begin = m_token.text.Begin();
                        const char* end = m_token.text.End();
                        int32_t base = 10;
                        if (*begin == '0' && m_token.text.Size() > 1)
                        {
                            ++begin;
                            switch (*begin)
                            {
                                case 'x':
                                    ++begin;
                                    base = 16;
                                    break;
                                case 'o':
                                    ++begin;
                                    base = 8;
                                    break;
                                case 'b':
                                    ++begin;
                                    base = 2;
                                    break;
                            }
                        }
                        const uint64_t value = String::ToInteger<uint64_t>(begin, end, base);
                        if (!m_handler->Uint(value))
                            return SetError(TomlReadError::Cancelled);
                    }

                    return NextDecl();
                }
                case TomlToken::OpenCurlyBracket:
                {
                    return ConsumeInlineTable();
                }
                case TomlToken::OpenSquareBracket:
                {
                    return ConsumeInlineArray();
                }
                case TomlToken::Identifier:
                {
                    if (m_token.text == "false")
                    {
                        if (!m_handler->Bool(false))
                            return SetError(TomlReadError::Cancelled);
                        return NextDecl();
                    }

                    if (m_token.text == "true")
                    {
                        if (!m_handler->Bool(true))
                            return SetError(TomlReadError::Cancelled);
                        return NextDecl();
                    }

                    return SetError(TomlReadError::InvalidToken);
                }
                case TomlToken::String:
                {
                    if (!m_handler->String(m_token.text))
                        return SetError(TomlReadError::Cancelled);

                    return NextDecl();
                }
                default:
                    return SetError(TomlReadError::InvalidToken);
            }
        }

    private:
        TomlLexer m_lexer{};
        TomlLexer::Token m_token{};
        TomlReadResult m_result{};
        TomlReader::Handler* m_handler{ nullptr };
        Vector<StringView> m_pathBuffer{};
    };

    // --------------------------------------------------------------------------------------------
    TomlReadResult TomlReader::Read(StringView data, Handler& handler)
    {
        TomlParser parser;
        return parser.Parse(data, handler);
    }
}
