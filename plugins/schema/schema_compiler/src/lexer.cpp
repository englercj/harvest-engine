// Copyright Chad Engler

#include "lexer.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/types.h"

namespace he::schema
{
    static bool IsIdentifierStart(char c)
    {
        return IsAlpha(c) || c == '_';
    }

    static bool IsIdentifier(char c)
    {
        return IsIdentifierStart(c) || IsNumeric(c);
    }

    bool Lexer::Reset(const char* src)
    {
        m_cursor = src;
        m_nextTokenStart = src;
        m_lineStart = src;
        m_nextLineStart = src;
        m_nextError = nullptr;
        m_nextToken = TokenType::None;

        if (String::IsEmpty(src))
        {
            m_nextError = "Input schema is empty";
            return false;
        }

        if (!SkipBOM())
            return false;

        // hydrate the first token
        NextToken();

        Token firstToken = PeekNextToken();

        if (firstToken.type == TokenType::Eof)
        {
            m_nextError = "Input schema is empty";
            return false;
        }

        return firstToken.type != TokenType::Error;
    }

    Lexer::Token Lexer::PeekNextToken()
    {
        HE_ASSERT(m_nextTokenStart >= m_nextLineStart);

        Token token;
        token.type = m_nextError ? TokenType::Error : m_nextToken;
        token.text = { m_nextTokenStart, m_cursor };
        token.line = m_line;
        token.column = static_cast<uint32_t>(m_nextTokenStart - m_nextLineStart) + 1;
        token.error = m_nextError;
        return token;
    }

    Lexer::Token Lexer::NextToken()
    {
        Token token = PeekNextToken();

        m_lineStart = m_nextLineStart;
        m_nextTokenStart = m_cursor;
        m_nextError = nullptr;
        m_nextToken = LexToken();
        return token;
    }

    Lexer::TokenType Lexer::LexToken()
    {
        while (true)
        {
            char c = *m_cursor++;
            switch (c)
            {
                case '\0':
                    --m_cursor;
                    return TokenType::Eof;
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
                case '@':
                    return LexOrdinal();
                case '0':
                {
                    if (*m_cursor == 'x')
                    {
                        ++m_cursor;

                        if (*m_cursor == '"')
                        {
                            ++m_cursor;
                            return LexBlob();
                        }

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
                    return LexDecNum();
                case '-':
                    if (*m_cursor == '>')
                    {
                        ++m_cursor;
                        return TokenType::Arrow;
                    }
                    return LexDecNum();
                case '"':
                    return LexString();
                case '{':
                    return TokenType::OpenCurlyBracket;
                case '}':
                    return TokenType::CloseCurlyBracket;
                case '[':
                    return TokenType::OpenSquareBracket;
                case ']':
                    return TokenType::CloseSquareBracket;
                case '$':
                    return TokenType::Dollar;
                case '.':
                    return TokenType::Dot;
                case '<':
                    return TokenType::OpenAngleBracket;
                case '>':
                    return TokenType::CloseAngleBracket;
                case '(':
                    return TokenType::OpenParens;
                case ')':
                    return TokenType::CloseParens;
                case ':':
                    return TokenType::Colon;
                case ';':
                    return TokenType::Semicolon;
                case ',':
                    return TokenType::Comma;
                case '=':
                    return TokenType::Equals;
                case '*':
                    return TokenType::Asterisk;
                case '/':
                    if (*m_cursor == '/')
                    {
                        ++m_cursor;
                        return LexComment();
                    }

                    // otherwise fall through
                    [[fallthrough]];
                default:
                    if (IsIdentifierStart(c))
                        return LexIdentifier();

                    m_nextError = "Encountered unknown token";
                    return TokenType::Error;
            }
        }
    }

    Lexer::TokenType Lexer::LexOrdinal()
    {
        while (IsHex(*m_cursor) || IsNumeric(*m_cursor) || *m_cursor == 'x')
            ++m_cursor;

        return TokenType::Ordinal;
    }

    Lexer::TokenType Lexer::LexBlob()
    {
        while (true)
        {
            char c = *m_cursor++;

            if (!IsHex(c) && c != ' ' && c != '"')
            {
                m_nextError = "Illegal character in blob literal";
                return TokenType::Error;
            }

            if (c == '"')
                return TokenType::Blob;
        }
    }

    Lexer::TokenType Lexer::LexBinNum()
    {
        while (*m_cursor == '0' || *m_cursor == '1')
            ++m_cursor;

        return TokenType::Integer;
    }

    Lexer::TokenType Lexer::LexDecNum()
    {
        bool hasDot = false;
        while (IsNumeric(*m_cursor) || *m_cursor == '.')
        {
            if (*m_cursor == '.')
            {
                // When true there are multiple dots which is not valid
                if (hasDot)
                {
                    m_nextError = "Invalid number literal";
                    return TokenType::Error;
                }

                hasDot = true;
            }

            ++m_cursor;
        }

        return hasDot ? TokenType::Float : TokenType::Integer;
    }

    Lexer::TokenType Lexer::LexHexNum()
    {
        while (IsHex(*m_cursor))
            ++m_cursor;

        return TokenType::Integer;
    }

    Lexer::TokenType Lexer::LexOctNum()
    {
        while (*m_cursor >= '0' && *m_cursor <= '7')
            ++m_cursor;

        return TokenType::Integer;
    }

    Lexer::TokenType Lexer::LexString()
    {
        while (true)
        {
            char c = *m_cursor++;

            if (c < ' ' && static_cast<signed char>(c) >= 0)
            {
                m_nextError = "Illegal character in string literal";
                return TokenType::Error;
            }

            if (c == '"')
                return TokenType::String;
        }
    }

    Lexer::TokenType Lexer::LexComment()
    {
        while (*m_cursor && *m_cursor != '\n' && *m_cursor != '\r')
            ++m_cursor;

        return TokenType::Comment;
    }

    Lexer::TokenType Lexer::LexIdentifier()
    {
        while (*m_cursor && IsIdentifier(*m_cursor))
            ++m_cursor;

        return TokenType::Identifier;
    }

    bool Lexer::SkipBOM()
    {
        if (static_cast<uint8_t>(*m_cursor) != 0xef)
            return true;

        ++m_cursor;
        if (static_cast<uint8_t>(*m_cursor) != 0xbb)
        {
            m_nextError = "Invalid utf-8 byte order mark";
            return false;
        }

        ++m_cursor;
        if (static_cast<uint8_t>(*m_cursor) != 0xbf)
        {
            m_nextError = "Invalid utf-8 byte order mark";
            return false;
        }

        ++m_cursor;
        return true;
    }
}

namespace he
{
    template <>
    const char* AsString(schema::Lexer::TokenType type)
    {
        switch (type)
        {
            case schema::Lexer::TokenType::None: return "None";
            case schema::Lexer::TokenType::Error: return "Error";
            case schema::Lexer::TokenType::Arrow: return "Arrow";
            case schema::Lexer::TokenType::Asterisk: return "Asterisk";
            case schema::Lexer::TokenType::Blob: return "Blob";
            case schema::Lexer::TokenType::Colon: return "Colon";
            case schema::Lexer::TokenType::Comma: return "Comma";
            case schema::Lexer::TokenType::Comment: return "Comment";
            case schema::Lexer::TokenType::CloseAngleBracket: return "CloseAngleBracket";
            case schema::Lexer::TokenType::CloseCurlyBracket: return "CloseCurlyBracket";
            case schema::Lexer::TokenType::CloseParens: return "CloseParens";
            case schema::Lexer::TokenType::CloseSquareBracket: return "CloseSquareBracket";
            case schema::Lexer::TokenType::Dollar: return "Dollar";
            case schema::Lexer::TokenType::Dot: return "Dot";
            case schema::Lexer::TokenType::Eof: return "Eof";
            case schema::Lexer::TokenType::Equals: return "Equals";
            case schema::Lexer::TokenType::Float: return "Float";
            case schema::Lexer::TokenType::Identifier: return "Identifier";
            case schema::Lexer::TokenType::Integer: return "Integer";
            case schema::Lexer::TokenType::OpenAngleBracket: return "OpenAngleBracket";
            case schema::Lexer::TokenType::OpenCurlyBracket: return "OpenCurlyBracket";
            case schema::Lexer::TokenType::OpenParens: return "OpenParens";
            case schema::Lexer::TokenType::OpenSquareBracket: return "OpenSquareBracket";
            case schema::Lexer::TokenType::Ordinal: return "Ordinal";
            case schema::Lexer::TokenType::Plus: return "Plus";
            case schema::Lexer::TokenType::Semicolon: return "Semicolon";
            case schema::Lexer::TokenType::String: return "String";
            case schema::Lexer::TokenType::_Count: return "_Count";
        }

        return "<unknown>";
    }
}
