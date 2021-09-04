// Copyright Chad Engler

#include "he/schema/lexer.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
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

    Lexer::Lexer(Allocator& allocator)
        : m_error(allocator)
    {}

    bool Lexer::Reset(StringView src)
    {
        if (src.IsEmpty())
        {
            m_error = "Input schema is empty";
            return false;
        }

        m_cursor = src.Data();
        m_nextTokenStart = src.Data();
        m_lineStart = src.Data();
        m_nextLineStart = src.Data();
        m_nextToken = TokenType::None;
        m_error.Clear();

        if (!SkipBOM())
            return false;

        // hydrate the first token
        if (GetNextToken().type == TokenType::Eof)
        {
            m_error = "Input schema is empty";
            return false;
        }

        return true;
    }

    Lexer::Token Lexer::PeekNextToken()
    {
        HE_ASSERT(m_nextTokenStart >= m_lineStart);

        Token token;
        token.type = m_nextToken;
        token.text = { m_nextTokenStart, m_cursor };
        token.line = m_line;
        token.column = static_cast<uint32_t>(m_nextTokenStart - m_lineStart) + 1;
        return token;
    }

    Lexer::Token Lexer::GetNextToken()
    {
        Token token = PeekNextToken();

        m_lineStart = m_nextLineStart;
        m_nextTokenStart = m_cursor;
        m_nextToken = LexToken();

        return token;
    }

    Lexer::TokenType Lexer::LexToken()
    {
        char c = *m_cursor++;
        switch (c)
        {
            case '\0':
                --m_cursor;
                return TokenType::Eof;
            case ' ':
            case '\t':
            case '\v':
            case '\f':
            case '\r':
                return LexWhitespace();
            case '\n':
                return LexNewline();
            case '0':
            {
                if (*m_cursor == 'x' || *m_cursor == 'X')
                {
                    ++m_cursor;
                    return LexHexNum();
                }

                if (*m_cursor == 'x' || *m_cursor == 'X')
                {
                    ++m_cursor;
                    return LexBinNum();
                }

                return LexOctNum();
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
                return LexDecNum();
            case '+':
                return TokenType::Plus;
            case '-':
                if (*m_cursor == '>')
                {
                    ++m_cursor;
                    return TokenType::Arrow;
                }
                return TokenType::Minus;
            case '*':
                return TokenType::Asterisk;
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
            case '.':
                return TokenType::Dot;
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

                m_error = "Encountered unexpected token";
                return TokenType::Error;
        }
    }

    Lexer::TokenType Lexer::LexWhitespace()
    {
        while (*m_cursor == ' ' || *m_cursor == '\t' || *m_cursor == '\v' || *m_cursor == '\f' || *m_cursor == '\r')
            ++m_cursor;

        return TokenType::Whitespace;
    }

    Lexer::TokenType Lexer::LexNewline()
    {
        ++m_line;
        m_nextLineStart = m_cursor;
        return TokenType::Newline;
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
                    m_error = "Invalid number literal";
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
                m_error = "Illegal character in string literal";
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
            m_error = "Invalid utf-8 byte order mark";
            return false;
        }

        ++m_cursor;
        if (static_cast<uint8_t>(*m_cursor) != 0xbf)
        {
            m_error = "Invalid utf-8 byte order mark";
            return false;
        }

        ++m_cursor;
        return true;
    }

    const char* AsString(Lexer::TokenType type)
    {
        switch (type)
        {
            case Lexer::TokenType::None: return "None";
            case Lexer::TokenType::Arrow: return "Arrow";
            case Lexer::TokenType::Asterisk: return "Asterisk";
            case Lexer::TokenType::Colon: return "Colon";
            case Lexer::TokenType::Comma: return "Comma";
            case Lexer::TokenType::Comment: return "Comment";
            case Lexer::TokenType::CloseAngleBracket: return "CloseAngleBracket";
            case Lexer::TokenType::CloseCurlyBracket: return "CloseCurlyBracket";
            case Lexer::TokenType::CloseParens: return "CloseParens";
            case Lexer::TokenType::CloseSquareBracket: return "CloseSquareBracket";
            case Lexer::TokenType::Eof: return "Eof";
            case Lexer::TokenType::Equals: return "Equals";
            case Lexer::TokenType::Error: return "Error";
            case Lexer::TokenType::Float: return "Float";
            case Lexer::TokenType::Identifier: return "Identifier";
            case Lexer::TokenType::Integer: return "Integer";
            case Lexer::TokenType::Minus: return "Minus";
            case Lexer::TokenType::Newline: return "Newline";
            case Lexer::TokenType::OpenAngleBracket: return "OpenAngleBracket";
            case Lexer::TokenType::OpenCurlyBracket: return "OpenCurlyBracket";
            case Lexer::TokenType::OpenParens: return "OpenParens";
            case Lexer::TokenType::OpenSquareBracket: return "OpenSquareBracket";
            case Lexer::TokenType::Plus: return "Plus";
            case Lexer::TokenType::Semicolon: return "Semicolon";
            case Lexer::TokenType::String: return "String";
            case Lexer::TokenType::Whitespace: return "Whitespace";
            case Lexer::TokenType::_Count: return "_Count";
        }

        return "<unknown>";
    }
}
