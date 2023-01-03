// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he::schema
{
    /// Transforms a schema text blob into a token stream.
    class Lexer
    {
    public:
        enum class TokenType : uint8_t
        {
            None,
            Error,

            Arrow,
            Asterisk,
            Blob,
            Colon,
            Comma,
            Comment,
            CloseAngleBracket,
            CloseCurlyBracket,
            CloseParens,
            CloseSquareBracket,
            Dollar,
            Dot,
            Eof,
            Equals,
            Float,
            Identifier,
            Integer,
            OpenAngleBracket,
            OpenCurlyBracket,
            OpenParens,
            OpenSquareBracket,
            Ordinal,
            Plus,
            Semicolon,
            String,

            _Count,
        };

        struct Token
        {
            TokenType type{ TokenType::None };
            StringView text{};
            uint32_t line{ 0 };
            uint32_t column{ 0 };
            const char* error{ nullptr };
        };

    public:
        bool Reset(const char* src);

        Token NextToken();
        Token PeekNextToken();

    private:
        TokenType LexToken();
        TokenType LexOrdinal();
        TokenType LexBlob();
        TokenType LexLeadingZero();
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

        const char* m_nextError{ nullptr };

        uint32_t m_line{ 1 };
        TokenType m_nextToken{ TokenType::None };
    };
}
