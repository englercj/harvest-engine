// Copyright Chad Engler

// TODO: Unreleased features as of v1.0.0 that are not yet supported:
// - Clarify Unicode and UTF-8 references.
// - Clarify where and how dotted keys define tables.
// - Add new \e shorthand for the escape character.
// - Add \x00 notation to basic strings.
// - Seconds in Date-Time and Time values are now optional.
// - Allow non-English scripts in unquoted (bare) keys
// - Clarify newline normalization in multi-line literal strings.

#include "he/core/toml_reader.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/string_fmt.h"
#include "he/core/vector.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    static bool IsIdentifier(char ch)
    {
        return IsAlpha(ch) || IsNumeric(ch) || ch == '_';
    }

    static bool IsInvalidInComment(char ch)
    {
        return ch == '\x00' || (ch >= '\x0A' && ch <= '\x0D');
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
            String text{};
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
            m_decodedString.Clear();

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

            if (token.kind == TomlToken::String)
                token.text = m_decodedString;
            else
                token.text.Assign(m_nextTokenStart, m_cursor);

            token.line = m_line;
            token.column = static_cast<uint32_t>(m_nextTokenStart - m_nextLineStart) + 1;
            token.error = m_nextError;
            return token;
        }

        void PushNewlinesInvalid() { ++m_newlinesAreInvalid; }
        void PopNewlinesInvalid() { HE_ASSERT(m_newlinesAreInvalid > 0); --m_newlinesAreInvalid; }

    private:
        bool CheckEof(uint32_t offset = 1)
        {
            if (m_cursor > (m_end - offset))
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
                    case '\r':
                    case '\n':
                        --m_cursor;
                        if (!SkipNewline())
                            return TomlToken::Error;
                        break;
                    case ' ':
                    case '\t':
                    case '\v':
                    case '\f':
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
                        ++m_nextTokenStart;
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
            return LexGenericString('"', true);
        }

        TomlToken LexLiteralString()
        {
            return LexGenericString('\'', false);
        }

        TomlToken LexGenericString(char quote, bool decodeEscapeCharacters)
        {
            if (!CheckEof())
                return TomlToken::Error;

            m_decodedString.Clear();

            // Check opening quotes for multiline string. We've already consumed one quote to get
            // here, so we check for a second and third quote.
            bool isMultiline = false;
            if (*m_cursor == quote)
            {
                ++m_cursor;
                if (m_cursor < m_end && *m_cursor == quote)
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

            if (!CheckEof())
                return TomlToken::Error;

            // Skip a single newline if it immediately follows the opening of a multiline string
            if (isMultiline)
            {
                if (!SkipNewline())
                    return TomlToken::Error;
            }

            // Decode the string into our buffer
            while (true)
            {
                if (!CheckEof())
                    return TomlToken::Error;

                // Unescaped newlines are allowed in multiline strings.
                // Here we also normalize `\r\n` sequences to `\n`.
                if (isMultiline && (*m_cursor == '\r' || *m_cursor == '\n'))
                {
                    if (!SkipNewline())
                        return TomlToken::Error;

                    m_decodedString.PushBack('\n');
                    continue;
                }

                char ch = *m_cursor++;

                // Tab characters are explicitly allowed.
                if (ch == '\t')
                {
                    m_decodedString.PushBack('\t');
                    continue;
                }

                // Unescaped control characters, except for the special cases handled above, are
                // not allowed in strings.
                if (ch < '\x20' || ch == '\x7F')
                {
                    m_nextError = TomlReadError::InvalidToken;
                    return TomlToken::Error;
                }

                // Check for string ending quotes. For multiline strings you can actually put
                // literal quotes in the string. Only a series of three quotes (""") ends it.
                if (ch == quote)
                {
                    if (!isMultiline)
                        return TomlToken::String;

                    if (m_cursor < m_end && *m_cursor == quote)
                    {
                        ++m_cursor;

                        if (m_cursor < m_end && *m_cursor == quote)
                        {
                            ++m_cursor;
                            return TomlToken::String;
                        }
                        m_decodedString.PushBack(quote);
                    }
                    m_decodedString.PushBack(quote);
                    continue;
                }

                // Handle escaped characters
                if (decodeEscapeCharacters && ch == '\\')
                {
                    if (!CheckEof())
                        return TomlToken::Error;

                    switch (*m_cursor)
                    {
                        case 'b': m_decodedString.PushBack('\b'); ++m_cursor; break;
                        case 't': m_decodedString.PushBack('\t'); ++m_cursor; break;
                        case 'n': m_decodedString.PushBack('\n'); ++m_cursor; break;
                        case 'f': m_decodedString.PushBack('\f'); ++m_cursor; break;
                        case 'r': m_decodedString.PushBack('\r'); ++m_cursor; break;
                        case 'e': m_decodedString.PushBack('\x1B'); ++m_cursor; break;
                        case '"': m_decodedString.PushBack('"'); ++m_cursor; break;
                        case '\\': m_decodedString.PushBack('\\'); ++m_cursor; break;
                        case '\r':
                        case '\n':
                        {
                            // Escaped newlines indicate that we ignore all whitespace until the
                            // next non-whitespace character
                            while (m_cursor < m_end && IsWhitespace(*m_cursor))
                            {
                                if (*m_cursor == '\r' || *m_cursor == '\n')
                                {
                                    if (!SkipNewline())
                                        return TomlToken::Error;
                                }
                                else
                                {
                                    ++m_cursor;
                                }
                            }
                            break;
                        }
                        case 'x':
                        {
                            if (!LexUnicodeCodePoint(2))
                                return TomlToken::Error;
                            break;
                        }
                        case 'u':
                        {
                            if (!LexUnicodeCodePoint(4))
                                return TomlToken::Error;
                            break;
                        }
                        case 'U':
                        {
                            if (!LexUnicodeCodePoint(8))
                                return TomlToken::Error;
                            break;
                        }
                        default:
                            m_nextError = TomlReadError::InvalidToken;
                            return TomlToken::Error;
                    }

                    continue;
                }

                // Totally normal character that just gets pushed into our decode buffer
                m_decodedString.PushBack(ch);
            }
        }

        bool LexUnicodeCodePoint(uint32_t len)
        {
            if (!CheckEof(len))
                return false;

            // Validate each of the characters and read their values into codepoint
            uint32_t codepoint = 0;
            for (uint32_t i = 0; i < len; ++i)
            {
                if (!IsHex(m_cursor[i]))
                {
                    m_nextError = TomlReadError::InvalidToken;
                    return false;
                }
                codepoint = (codepoint << 4) + HexToNibble(m_cursor[i]);
            }

            // Surrogate pairs are not allowed
            if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
            {
                m_nextError = TomlReadError::InvalidToken;
                return false;
            }

            // Valid Unicode range for TOML is <= 0x10FFFF
            if (codepoint > 0x10FFFF)
            {
                m_nextError = TomlReadError::InvalidToken;
                return false;
            }

            m_cursor += len;

            if (codepoint <= 0x7F)
            {
                m_decodedString.PushBack(static_cast<uint8_t>(codepoint));
            }
            else if (codepoint <= 0x7FF)
            {
                m_decodedString.PushBack(static_cast<uint8_t>(0xC0 | (codepoint >> 6)));
                m_decodedString.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            else if (codepoint <= 0xFFFF)
            {
                m_decodedString.PushBack(static_cast<uint8_t>(0xE0 | (codepoint >> 12)));
                m_decodedString.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                m_decodedString.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            else
            {
                m_decodedString.PushBack(static_cast<uint8_t>(0xF0 | (codepoint >> 18)));
                m_decodedString.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
                m_decodedString.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                m_decodedString.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }

            return true;
        }

        bool SkipNewline()
        {
            if (*m_cursor == '\r')
            {
                ++m_cursor; // skip '\r'
                if (!CheckEof() || *m_cursor != '\n')
                {
                    m_nextError = TomlReadError::InvalidToken;
                    return false;
                }
            }

            if (*m_cursor == '\n')
            {
                if (m_newlinesAreInvalid)
                {
                    m_nextError = TomlReadError::InvalidToken;
                    return false;
                }

                ++m_cursor;
                ++m_line;
                m_nextLineStart = m_cursor;
                m_nextTokenStart = m_cursor;
            }

            return true;
        }

        TomlToken LexComment()
        {
            while (*m_cursor && *m_cursor != '\n' && *m_cursor != '\r')
            {
                if (IsInvalidInComment(*m_cursor))
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

        uint32_t m_newlinesAreInvalid{ 0 };

        TomlReadError m_nextError{ TomlReadError::None };

        uint32_t m_line{ 1 };
        TomlToken m_nextToken{ TomlToken::None };
        String m_decodedString{};
    };

    // --------------------------------------------------------------------------------------------
    class TomlParser
    {
    public:
        TomlReadResult Parse(StringView src, TomlReader::Handler& handler)
        {
            m_result = {};
            m_handler = &handler;
            m_cursor = src.Begin();
            m_end = src.End();
            m_lineStart = m_cursor;
            m_line = 1;
            m_pathBuffer.Clear();

            if (!SkipBOM())
            {
                SetError(TomlReadError::InvalidBom);
                return m_result;
            }

            m_handler->StartDocument();

            if (!ParseExpression())
                return m_result;

            while (!AtEnd())
            {
                if (!ParseNewLine())
                    return m_result;

                if (!ParseExpression())
                    return m_result;
            }

            m_handler->EndDocument();

            return m_result;
        }

    private:
        bool SetError(TomlReadError error, char expected = '\0')
        {
            const uint32_t column = static_cast<uint32_t>(m_cursor - m_lineStart);
            m_result = { error, m_line, column, expected };
            return false;
        }

        [[nodiscard]] bool AtEnd() const { return m_cursor >= m_end || *m_cursor == '\0'; }

        [[nodiscard]] bool SkipBOM()
        {
            if (static_cast<uint8_t>(*m_cursor) != 0xef)
                return true;

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbb)
                return SetError(TomlReadError::InvalidBom);

            ++m_cursor;
            if (static_cast<uint8_t>(*m_cursor) != 0xbf)
                return SetError(TomlReadError::InvalidBom);

            ++m_cursor;
            return true;
        }

        void SkipSpaces()
        {
            while (!AtEnd() && (*m_cursor == ' ' || *m_cursor == '\t'))
                ++m_cursor;
        }

        void SkipSpacesAndNewlines()
        {
            while (!AtEnd())
            {
                SkipSpaces();

                if (AtEnd())
                    break;

                if (*m_cursor == '#')
                {
                    ParseComment();
                    ParseNewLine();
                }
                else if (*m_cursor == '\r' || *m_cursor == '\n')
                {
                    ParseNewLine();
                }
                else
                {
                    break;
                }
            }
        }

        // [[nodiscard]] bool At(char ch) const
        // {
        //     return !AtEnd() && *m_cursor == ch;
        // }

        [[nodiscard]] bool Consume(char ch)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            if (*m_cursor != ch)
                return SetError(TomlReadError::InvalidToken, ch);

            ++m_cursor;
            return true;
        }

        [[nodiscard]] bool ConsumeOneOf(const char* chars)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            while (*chars)
            {
                if (*m_cursor == *chars)
                {
                    ++m_cursor;
                    return true;
                }
                ++chars;
            }

            return SetError(TomlReadError::InvalidToken);
        }

        [[nodiscard]] static bool IsValidInKey(char ch)
        {
            return IsAlphaNum(ch) || ch == '_' || ch == '-';
        }

    private:
        bool ParseNewLine()
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            if (*m_cursor == '\r')
            {
                if (!Consume('\r'))
                    return false;

                if (!Consume('\n'))
                    return SetError(TomlReadError::InvalidNewline);
            }
            else if (!Consume('\n'))
            {
                return false;
            }

            ++m_line;
            m_lineStart = m_cursor;
            return true;
        }

        bool ParseComment()
        {
            if (!Consume('#'))
                return false;

            const char* begin = m_cursor;
            while (!AtEnd() && *m_cursor != '\r' && *m_cursor != '\n')
            {
                const char ch = *m_cursor;
                if (ch == '\x00' || (ch >= '\x0A' && ch <= '\x0D'))
                    return SetError(TomlReadError::InvalidToken);

                ++m_cursor;
            }

            m_handler->Comment(StringView(begin, m_cursor));
            return true;
        }

        bool ParseExpression()
        {
            SkipSpaces();

            if (AtEnd())
                return true;

            switch (*m_cursor)
            {
                case '\r':
                case '\n':
                    return;
                case '#':
                    return ParseComment();
                case '[':
                {
                    if (!ParseTableHeader())
                        return false;
                    SkipSpaces();
                    if (!AtEnd() && *m_cursor == '#')
                        return ParseComment();
                    break;
                }
                default:
                    if (!ParseKeyValuePair())
                        return false;

                    SkipSpaces();
                    if (!AtEnd() && *m_cursor == '#')
                        return ParseComment();
            }
        }

        bool ParseTableHeader()
        {
            if (!Consume('['))
                return false;

            const bool isArray = *m_cursor == '[';
            if (isArray)
            {
                if (!Consume('['))
                    return false;
            }

            m_pathBuffer.Clear();
            if (!ParseKey())
                return false;

            SkipSpaces();

            if (!Consume(']'))
                return false;

            if (isArray)
            {
                if (!Consume(']'))
                    return false;
            }

            m_handler->StartTable(m_pathBuffer, isArray);
            return true;
        }

        bool ParseKeyValuePair()
        {
            if (!ParseKey())
                return false;

            m_handler->Key(m_pathBuffer);

            SkipSpaces();

            if (!Consume('='))
                return false;

            SkipSpaces();

            if (!ParseValue())
                return false;

            m_pathBuffer.PopBack();
            return true;
        }

        bool ParseKey()
        {
            while (true)
            {
                SkipSpaces();

                if (AtEnd())
                    return SetError(TomlReadError::UnexpectedEof);

                String& dst = m_pathBuffer.EmplaceBack();

                if (*m_cursor == '"')
                {
                    if (!ParseBasicString(dst, false))
                        return false;
                }
                else if (*m_cursor == '\'')
                {
                    if (!ParseLiteralString(dst, false))
                        return false;
                }
                else
                {
                    const char* begin = m_cursor;

                    while (!AtEnd() && IsValidInKey(*m_cursor))
                        ++m_cursor;

                    if (begin == m_cursor)
                        return SetError(TomlReadError::InvalidKey);

                    if (AtEnd())
                        return SetError(TomlReadError::UnexpectedEof);

                    dst.Assign(begin, m_cursor);
                }

                SkipSpaces();

                if (AtEnd() || *m_cursor != '.')
                    break;

                if (!Consume('.'))
                    return false;
            }
        }

        bool ParseValue()
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            switch (*m_cursor)
            {
                case '[':
                    return ParseInlineArray();
                case '{':
                    return ParseInlineTable();
                case '"':
                    if (!ParseBasicString(m_stringBuffer, true))
                        return false;
                    m_handler->String(m_stringBuffer);
                    return true;
                case '\'':
                    if (!ParseLiteralString(m_stringBuffer, true))
                        return false;
                    m_handler->String(m_stringBuffer);
                    return true;
                case 't':
                    return ParseTrue();
                case 'f':
                    return ParseFalse();
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
                    return ParseDateTimeOrNumber();
                case '+':
                case '-':
                    return ParseNumber();
                default:
                    return SetError(TomlReadError::InvalidToken);
            }
        }

        bool ParseInlineArray()
        {
            if (!Consume('['))
                return false;

            m_handler->StartArray();

            SkipSpacesAndNewlines();

            uint32_t count = 0;
            while (!AtEnd())
            {
                if (*m_cursor == ']')
                {
                    if (!Consume(']'))
                        return false;

                    m_handler->EndArray(count);
                    return true;
                }

                if (!ParseValue())
                    return false;

                ++count;
                SkipSpacesAndNewlines();

                if (*m_cursor != ',' && *m_cursor != ']')
                    return SetError(TomlReadError::InvalidToken);

                if (*m_cursor == ',')
                    SkipSpacesAndNewlines();
            }
        }

        bool ParseInlineTable()
        {
            if (!Consume('{'))
                return false;

            m_handler->StartTable(m_pathBuffer, false);

            SkipSpacesAndNewlines();

            uint32_t count = 0;
            while (!AtEnd())
            {
                if (*m_cursor == '}')
                {
                    if (!Consume('}'))
                        return false;

                    m_handler->EndTable(count);
                    return true;
                }

                if (!ParseKeyValuePair())
                    return false;

                ++count;
                SkipSpacesAndNewlines();

                if (*m_cursor != ',' && *m_cursor != '}')
                    return SetError(TomlReadError::InvalidToken);

                if (*m_cursor == ',')
                    SkipSpacesAndNewlines();
            }
        }

        bool ParseBasicString(String& dst, bool allowMultiline)
        {
            return ParseGenericString(dst, '"', true, allowMultiline);
        }

        bool ParseLiteralString(String& dst, bool allowMultiline)
        {
            return ParseGenericString(dst, '\'', false, allowMultiline);
        }

        bool ParseGenericString(String& dst, char quote, bool decodeEscapeCharacters, bool allowMultiline)
        {
            dst.Clear();

            if (!Consume(quote))
                return false;

            // Check opening quotes for multiline string. We've already consumed one quote to get
            // here, so we check for a second and third quote.
            bool isMultiline = false;
            if (*m_cursor == quote)
            {
                ++m_cursor;
                if (allowMultiline && m_cursor < m_end && *m_cursor == quote)
                {
                    ++m_cursor;
                    isMultiline = true;
                }
                else
                {
                    // empty string
                    return true;
                }
            }

            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            // Skip a single newline if it immediately follows the opening of a multiline string
            if (isMultiline && (*m_cursor == '\r' || *m_cursor == '\n'))
            {
                if (!ParseNewLine())
                    return false;
            }

            // Decode the string into our buffer
            while (true)
            {
                if (AtEnd())
                    return SetError(TomlReadError::UnexpectedEof);

                // Unescaped newlines are allowed in multiline strings.
                // Here we also normalize `\r\n` sequences to `\n`.
                if (isMultiline && (*m_cursor == '\r' || *m_cursor == '\n'))
                {
                    if (!ParseNewLine())
                        return false;

                    dst.PushBack('\n');
                    continue;
                }

                char ch = *m_cursor++;

                // Tab characters are explicitly allowed.
                if (ch == '\t')
                {
                    dst.PushBack('\t');
                    continue;
                }

                // Unescaped control characters, except for the special cases handled above, are
                // not allowed in strings.
                if (ch < '\x20' || ch == '\x7F')
                    return SetError(TomlReadError::InvalidControlChar);

                // Check for string ending quotes. For multiline strings you can actually put
                // literal quotes in the string. Only a series of three quotes (""") ends it.
                if (ch == quote)
                {
                    if (!isMultiline)
                        return true;

                    if (m_cursor < m_end && *m_cursor == quote)
                    {
                        ++m_cursor;

                        if (m_cursor < m_end && *m_cursor == quote)
                        {
                            ++m_cursor;
                            return true;
                        }
                        dst.PushBack(quote);
                    }
                    dst.PushBack(quote);
                    continue;
                }

                // Handle escaped characters
                if (decodeEscapeCharacters && ch == '\\')
                {
                    if (AtEnd())
                        return SetError(TomlReadError::UnexpectedEof);

                    switch (*m_cursor)
                    {
                        case 'b': dst.PushBack('\b'); ++m_cursor; break;
                        case 't': dst.PushBack('\t'); ++m_cursor; break;
                        case 'n': dst.PushBack('\n'); ++m_cursor; break;
                        case 'f': dst.PushBack('\f'); ++m_cursor; break;
                        case 'r': dst.PushBack('\r'); ++m_cursor; break;
                        case 'e': dst.PushBack('\x1B'); ++m_cursor; break;
                        case '"': dst.PushBack('"'); ++m_cursor; break;
                        case '\\': dst.PushBack('\\'); ++m_cursor; break;
                        case '\r':
                        case '\n':
                        {
                            // Escaped newlines indicate that we ignore all whitespace until the
                            // next non-whitespace character
                            while (m_cursor < m_end && IsWhitespace(*m_cursor))
                            {
                                if (*m_cursor == '\r' || *m_cursor == '\n')
                                {
                                    if (!ParseNewLine())
                                        return false;
                                }
                                else
                                {
                                    ++m_cursor;
                                }
                            }
                            break;
                        }
                        case 'x':
                        {
                            if (!ParseUnicodeCodePoint(dst, 2))
                                return false;
                            break;
                        }
                        case 'u':
                        {
                            if (!ParseUnicodeCodePoint(dst, 4))
                                return false;
                            break;
                        }
                        case 'U':
                        {
                            if (!ParseUnicodeCodePoint(dst, 8))
                                return false;
                            break;
                        }
                        default:
                            return SetError(TomlReadError::InvalidEscapeSequence);
                    }

                    continue;
                }

                // Totally normal character that just gets pushed into our decode buffer
                dst.PushBack(ch);
            }
        }

        bool ParseUnicodeCodePoint(String& dst, uint32_t len)
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            // Validate each of the characters and read their values into codepoint
            uint32_t codepoint = 0;
            for (uint32_t i = 0; i < len; ++i)
            {
                if (!IsHex(m_cursor[i]))
                    return SetError(TomlReadError::InvalidUnicode);

                codepoint = (codepoint << 4) + HexToNibble(m_cursor[i]);
            }

            // Surrogate pairs are not allowed
            if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
                return SetError(TomlReadError::InvalidUnicode);

            // Valid Unicode range for TOML is <= 0x10FFFF
            if (codepoint > 0x10FFFF)
                return SetError(TomlReadError::InvalidUnicode);

            m_cursor += len;

            if (codepoint <= 0x7F)
            {
                dst.PushBack(static_cast<uint8_t>(codepoint));
            }
            else if (codepoint <= 0x7FF)
            {
                dst.PushBack(static_cast<uint8_t>(0xC0 | (codepoint >> 6)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            else if (codepoint <= 0xFFFF)
            {
                dst.PushBack(static_cast<uint8_t>(0xE0 | (codepoint >> 12)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }
            else
            {
                dst.PushBack(static_cast<uint8_t>(0xF0 | (codepoint >> 18)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
                dst.PushBack(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
                dst.PushBack(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
            }

            return true;
        }

        bool ParseTrue()
        {
            if (!Consume('t') || !Consume('r') || !Consume('u') || !Consume('e'))
                return false;

            m_handler->Bool(true);
            return true;
        }

        bool ParseFalse()
        {
            if (!Consume('f') || !Consume('a') || !Consume('l') || !Consume('s') || !Consume('e'))
                return false;

            m_handler->Bool(false);
            return true;
        }

        bool ParseDateTimeOrNumber()
        {
            const uint32_t slack = static_cast<uint32_t>(m_end - m_cursor);

            if (slack >= 5)
            {
                // 4 digits and a dash indicates an "Offset Date-Time", "Local Date-Time",
                // or "Local Date".
                if (IsNumeric(m_cursor[0])
                    && IsNumeric(m_cursor[1])
                    && IsNumeric(m_cursor[2])
                    && IsNumeric(m_cursor[3])
                    && m_cursor[4] == '-')
                {
                    return ParseDateTime();
                }

                // 2 digits : 2 digits indicates a "Local Time"
                if (IsNumeric(m_cursor[0])
                    && IsNumeric(m_cursor[1])
                    && m_cursor[2] == ':'
                    && IsNumeric(m_cursor[3])
                    && IsNumeric(m_cursor[4]))
                {
                    return ParseTime();
                }
            }

            return ParseNumber();
        }

        bool ParseTime()
        {
            constexpr const char Nums[] = "0123456789";

            uint32_t hours = 0;
            uint32_t minutes = 0;
            double seconds = 0.0;

            // 2 digit hour
            const char* begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                 return false;

            hours = String::ToInteger<uint32_t>(begin, m_cursor);

            if (!Consume(':'))
                 return false;

            // 2 digit minute
            begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                 return false;

            minutes = String::ToInteger<uint32_t>(begin, m_cursor);

            // 2 digit seconds are optional
            if (!AtEnd() && *m_cursor == ':')
            {
                if (!Consume(':'))
                    return false;

                begin = m_cursor;
                if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                    return false;

                // Optional fractional seconds
                if (!AtEnd() && *m_cursor == '.')
                {
                    ++m_cursor;

                    if (!ConsumeOneOf(Nums))
                        return SetError(TomlReadError::InvalidDateTime);

                    while (!AtEnd() && IsNumeric(*m_cursor))
                        ++m_cursor;
                }

                seconds = String::ToFloat<double>(begin, m_cursor);
            }

            if (minutes > 60 || seconds > 60.0)
                return SetError(TomlReadError::InvalidDateTime);

            const Duration value = FromPeriod<Hours>(hours) + FromPeriod<Minutes>(minutes) + FromPeriod<Seconds>(seconds);
            m_handler->Time(value);
            return true;
        }

        bool ParseDateTime()
        {
            constexpr const char Nums[] = "0123456789";

            uint32_t year = 0;
            uint32_t month = 0;
            uint32_t day = 0;
            uint32_t hours = 0;
            uint32_t minutes = 0;
            double seconds = 0.0;
            Duration tzOffset = Duration_Zero;

            // 4 digit year
            const char* begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums) || !ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                return SetError(TomlReadError::InvalidDateTime);

            year = String::ToInteger<uint32_t>(begin, m_cursor);

            if (!Consume('-'))
                return SetError(TomlReadError::InvalidDateTime);

            // 2 digit month
            begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                 return SetError(TomlReadError::InvalidDateTime);

            month = String::ToInteger<uint32_t>(begin, m_cursor);

            if (!Consume('-'))
                 return SetError(TomlReadError::InvalidDateTime);

            // 2 digit day
            begin = m_cursor;
            if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                 return SetError(TomlReadError::InvalidDateTime);

            day = String::ToInteger<uint32_t>(begin, m_cursor);

            // Check if this include time information
            if (!AtEnd() && (*m_cursor == 'T' || *m_cursor == 't' || *m_cursor == ' '))
            {
                if (!ConsumeOneOf("tT "))
                    return SetError(TomlReadError::InvalidDateTime);

                // 2 digit hour
                begin = m_cursor;
                if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                    return SetError(TomlReadError::InvalidDateTime);

                hours = String::ToInteger<uint32_t>(begin, m_cursor);

                if (!Consume(':'))
                    return SetError(TomlReadError::InvalidDateTime);

                // 2 digit minute
                begin = m_cursor;
                if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                    return SetError(TomlReadError::InvalidDateTime);

                minutes = String::ToInteger<uint32_t>(begin, m_cursor);

                // 2 digit seconds are optional in TOML
                if (!AtEnd() && *m_cursor == ':')
                {
                    if (!Consume(':'))
                            return SetError(TomlReadError::InvalidDateTime);

                    begin = m_cursor;
                    if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                        return SetError(TomlReadError::InvalidDateTime);

                    // Optional fractional seconds
                    if (!AtEnd() && *m_cursor == '.')
                    {
                        ++m_cursor;

                        if (!ConsumeOneOf(Nums))
                            return SetError(TomlReadError::InvalidDateTime);

                        while (!AtEnd() && IsNumeric(*m_cursor))
                            ++m_cursor;
                    }

                    seconds = String::ToFloat<double>(begin, m_cursor);
                }

                // Optional timezone
                if (!AtEnd() && (*m_cursor == 'z' || *m_cursor == 'Z'))
                {
                    ++m_cursor;
                    tzOffset = Duration_Zero;
                }
                else if (!AtEnd() && (*m_cursor == '+' || *m_cursor == '-'))
                {
                    ++m_cursor;

                    // 2 digit hour offset
                    begin = m_cursor;
                    if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                        return SetError(TomlReadError::InvalidDateTime);

                    const uint32_t tzOffsetHours = String::ToInteger<uint32_t>(begin, m_cursor);

                    if (!Consume(':'))
                        return SetError(TomlReadError::InvalidDateTime);

                    // 2 digit minute offset
                    begin = m_cursor;
                    if (!ConsumeOneOf(Nums) || !ConsumeOneOf(Nums))
                        return SetError(TomlReadError::InvalidDateTime);

                    const uint32_t tzOffsetMinutes = String::ToInteger<uint32_t>(begin, m_cursor);

                    tzOffset = FromPeriod<Hours>(tzOffsetHours) + FromPeriod<Minutes>(tzOffsetMinutes);
                }
                else
                {
                    // When no offset is specified we must assume local timezone
                    tzOffset = GetLocalTimezoneOffset();
                }
            }
            else
            {
                // When there is no time information specified we must assume local timezone
                tzOffset = GetLocalTimezoneOffset();
            }

            // TODO: It is actually more complex than this because months have different numbers
            // of days, there are leap years, etc. Might need to use mktime or something similar.
            SystemTime dt{};
            dt += FromPeriod<Years>(year - 1970);
            dt += FromPeriod<Months>(month - 1);
            dt += FromPeriod<Days>(day - 1);
            dt += FromPeriod<Hours>(hours);
            dt += FromPeriod<Minutes>(minutes);
            dt += FromPeriod<Seconds>(seconds);
            dt += tzOffset;

            m_handler->DateTime(dt);
            return true;
        }

        bool ParseNumber()
        {
            if (AtEnd())
                return SetError(TomlReadError::UnexpectedEof);

            bool isSigned = false;
            if (*m_cursor == '-')
            {
                if (!Consume('-'))
                    return false;
                isSigned = true;
            }
            else if (*m_cursor == '+')
            {
                if (!Consume('+'))
                    return false;
            }


        }

















































    private:

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

        [[nodiscard]] bool TryConsumeOnly(TomlToken expected)
        {
            if (!At(expected))
                return false;

            return Next();
        }

        [[nodiscard]] bool Consume(TomlToken expected)
        {
            if (!Expect(expected))
                return false;

            return NextDecl();
        }

        [[nodiscard]] bool ConsumeOnly(TomlToken expected)
        {
            if (!Expect(expected))
                return false;

            return Next();
        }

        [[nodiscard]] bool ConsumeKeyPath()
        {
            m_lexer.PushNewlinesInvalid();

            m_pathBuffer.Clear();
            while (!AtEnd())
            {
                if (!At(TomlToken::Identifier) && !At(TomlToken::String) && !At(TomlToken::Integer) && !At(TomlToken::Float))
                    return SetError(TomlReadError::InvalidToken);

                if ((At(TomlToken::Integer) || At(TomlToken::Float)) && m_token.text[0] == '-')
                    return SetError(TomlReadError::InvalidToken);

                // Sometimes key names like `[[123.456]]` will lex as floats, so we have to
                // tokenize the dots manually in that case.
                if (At(TomlToken::Float))
                {
                    HE_ASSERT(!m_token.text.IsEmpty());

                    // Tokenizes based on the dot
                    const char* begin = m_token.text.Begin();
                    const char* end = begin;
                    while (begin < m_token.text.End())
                    {
                        end = String::Find(begin, '.');
                        end = end ? end : m_token.text.End();

                        m_pathBuffer.PushBack(StringView(begin, end));
                        begin = end + 1;
                    }

                    const bool endsWithDot = m_token.text.Back() == '.';

                    if (!NextDecl())
                        return false;

                    if (!endsWithDot && !TryConsume(TomlToken::Dot))
                        break;
                }
                else
                {
                    m_pathBuffer.PushBack(m_token.text);

                    if (!NextDecl())
                        return false;

                    if (!TryConsume(TomlToken::Dot))
                        break;
                }
            }
            m_lexer.PopNewlinesInvalid();

            return !AtEnd();
        }

        bool ConsumeRootTable()
        {
            m_handler->StartDocument();

            if (!NextDecl())
                return false;

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
            m_lexer.PushNewlinesInvalid();

            if (!ConsumeOnly(TomlToken::OpenSquareBracket))
                return false;

            const bool isArray = TryConsume(TomlToken::OpenSquareBracket);

            if (!ConsumeKeyPath())
                return false;

            if (!ConsumeOnly(TomlToken::CloseSquareBracket))
                return false;

            if (isArray && !ConsumeOnly(TomlToken::CloseSquareBracket))
                return false;

            m_lexer.PopNewlinesInvalid();

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
            while (!AtEnd() && !At(TomlToken::CloseCurlyBracket))
            {
                if (!ConsumeInlineTableEntry())
                    return false;

                ++keyCount;

                if (!TryConsume(TomlToken::Comma))
                    break;
            }

            if (!Consume(TomlToken::CloseCurlyBracket))
                return false;

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
            while (!AtEnd() && !At(TomlToken::CloseSquareBracket))
            {
                if (!ConsumeValue())
                    return false;

                ++count;

                if (!TryConsume(TomlToken::Comma))
                    break;
            };

            if (!Consume(TomlToken::CloseSquareBracket))
                return false;

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
                    const double value = String::ToFloat<double>(m_token.text.Begin(), m_token.text.End());

                    if (!m_handler->Float(value))
                        return SetError(TomlReadError::Cancelled);

                    return NextDecl();
                }
                case TomlToken::Integer:
                {
                    if (m_token.text[0] == '-')
                    {
                        const int64_t value = String::ToInteger<int64_t>(m_token.text.Begin(), m_token.text.End());
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
        TomlReadResult m_result{};
        TomlReader::Handler* m_handler{ nullptr };

        const char* m_cursor{ nullptr };
        const char* m_end{ nullptr };
        const char* m_lineStart{ nullptr };
        uint32_t m_line{ 1 };

        String m_stringBuffer{};
        Vector<String> m_pathBuffer{};
    };

    // --------------------------------------------------------------------------------------------
    TomlReadResult TomlReader::Read(StringView data, Handler& handler)
    {
        TomlParser parser;
        return parser.Parse(data, handler);
    }

    // --------------------------------------------------------------------------------------------
    template <>
    const char* AsString(TomlReadError x)
    {
        switch (x)
        {
            case TomlReadError::None: return "None";
            case TomlReadError::Cancelled: return "Cancelled";
            case TomlReadError::EmptyFile: return "EmptyFile";
            case TomlReadError::Eof: return "Eof";
            case TomlReadError::InvalidBom: return "InvalidBom";
            case TomlReadError::InvalidToken: return "InvalidToken";
        }

        return "<unknown>";
    }
}
