// Copyright Chad Engler

#pragma once

#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"

namespace he
{
    enum class TomlReadError : uint8_t
    {
        None,           ///< No error.
        Cancelled,      ///< The read was cancelled by the handler.
        EmptyFile,      ///< The input TOML data was empty.
        Eof,            ///< The input ended unexpectedly.
        InvalidBom,     ///< The utf-8 Byte Order Mark (BOM) of the file is not valid.
        InvalidToken,   ///< Encountered an unexpected token in the file.
    };

    struct TomlReadResult
    {
        TomlReadError error{ TomlReadError::None };
        uint32_t line{ 0 };
        uint32_t column{ 0 };

        [[nodiscard]] explicit operator bool() const { return error == TomlReadError::None; }
    };

    class TomlReader
    {
    public:
        class Handler
        {
        public:
            virtual ~Handler() = default;

            virtual bool StartDocument() = 0;
            virtual bool EndDocument() = 0;

            virtual bool Comment(StringView value) = 0;

            // Primative values
            virtual bool Bool(bool value) = 0;
            virtual bool Int(int64_t value) = 0;
            virtual bool Uint(uint64_t value) = 0;
            virtual bool Float(double value) = 0;
            virtual bool String(StringView value) = 0;
            // TODO: DateTime support

            // Tables
            virtual bool StartTable(Span<const he::String> path, bool isArray) = 0;
            virtual bool Key(Span<const he::String> path)= 0;
            virtual bool EndTable(uint32_t keyCount) = 0;

            // Arrays
            virtual bool StartArray() = 0;
            virtual bool EndArray(uint32_t length) = 0;
        };

    public:
        TomlReadResult Read(StringView data, Handler& handler);
    };
}
