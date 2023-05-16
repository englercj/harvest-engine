// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he
{
    enum class TomlReadError : uint8_t
    {
        None,                   ///< No error.
        Cancelled,              ///< The read was cancelled by the handler.
        UnexpectedEof,          ///< The input ended unexpectedly.
        InvalidBom,             ///< The utf-8 Byte Order Mark (BOM) of the file is not valid.
        InvalidNewline,         ///< A carriage return without a following newline (`\r` with no subsequent `\n`).
        InvalidUnicode,         ///< A unicode sequence (`\x12`, `\u1234`, `\U12345678`) was invalid.
        InvalidEscapeSequence,  ///< An escape sequence (`\n`, `\t`, `\"`, etc) was invalid.
        InvalidControlChar,     ///< A control character was encountered somewhere it wasn't expected (like in a string).
        InvalidKey,             ///< The name of a key contains invalid characters.
        InvalidDateTime,        ///< The format of a date or time value is invalid.
        InvalidNumber,          ///< Format of a number is invalid. For example, a negative hex number.
        InvalidToken,           ///< Encountered an unexpected token in the file.
    };

    struct TomlReadResult
    {
        TomlReadError error{ TomlReadError::None };
        uint32_t line{ 0 };
        uint32_t column{ 0 };

        /// When error is InvalidToken, this was the character that was expected instead.
        /// Otherwise this value is just '\0'.
        char expected{ '\0' };

        [[nodiscard]] explicit operator bool() const { return error == TomlReadError::None; }
    };

    class TomlReader
    {
    public:
        explicit TomlReader(Allocator& allocator = Allocator::GetDefault()) noexcept;

        class Handler
        {
        public:
            virtual ~Handler() = default;

            virtual bool StartDocument() = 0;
            virtual bool EndDocument() = 0;

            virtual bool Comment(StringView value) = 0;

            // Primitive values
            virtual bool Bool(bool value) = 0;
            virtual bool Int(int64_t value) = 0;
            virtual bool Uint(uint64_t value) = 0;
            virtual bool Float(double value) = 0;
            virtual bool String(StringView value) = 0;
            virtual bool DateTime(SystemTime value) = 0;
            virtual bool Time(Duration value) = 0;

            // Tables
            virtual bool Table(Span<const he::String> path, bool isArray) = 0;
            virtual bool Key(Span<const he::String> path)= 0;

            // Inline Tables
            virtual bool StartInlineTable() = 0;
            virtual bool EndInlineTable(uint32_t length) = 0;

            // Arrays
            virtual bool StartArray() = 0;
            virtual bool EndArray(uint32_t length) = 0;
        };

    public:
        TomlReadResult Read(StringView data, Handler& handler);

    private:
        Allocator& m_allocator;
    };
}
