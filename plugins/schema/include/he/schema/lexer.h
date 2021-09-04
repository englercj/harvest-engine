// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he::schema
{
    /// The lexer takes a schema text blob as input and processes it into a token stream.
    class Lexer
    {
    public:
        enum class TokenType : uint8_t
        {
            None,

            Arrow,
            Asterisk,
            Colon,
            Comma,
            Comment,
            CloseAngleBracket,
            CloseCurlyBracket,
            CloseParens,
            CloseSquareBracket,
            Dot,
            Eof,
            Equals,
            Error,
            Float,
            Identifier,
            Integer,
            Minus,
            Newline,
            OpenAngleBracket,
            OpenCurlyBracket,
            OpenParens,
            OpenSquareBracket,
            Plus,
            Semicolon,
            String,
            Whitespace,

            _Count,
        };

        struct Token
        {
            TokenType type{ TokenType::None };
            StringView text{};
            uint32_t line{ 0 };
            uint32_t column{ 0 };
        };

    public:
        Lexer(Allocator& allocator);

        bool Reset(StringView src);

        Token PeekNextToken();
        Token GetNextToken();

        bool HasError() const { return !m_error.IsEmpty(); }
        const char* GetErrorText() { return m_error.Data(); }

    private:
        TokenType LexToken();
        TokenType LexWhitespace();
        TokenType LexNewline();
        TokenType LexBinNum();
        TokenType LexDecNum();
        TokenType LexHexNum();
        TokenType LexOctNum();
        TokenType LexString();
        TokenType LexComment();
        TokenType LexIdentifier();

        bool SkipBOM();

    private:
        const char* m_cursor{ nullptr };
        const char* m_nextTokenStart{ nullptr };

        const char* m_lineStart{ nullptr };
        const char* m_nextLineStart{ nullptr };

        uint32_t m_line{ 1 };

        TokenType m_nextToken{ TokenType::None };

        String m_error;
    };

    const char* AsString(Lexer::TokenType type);
}
