// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string_builder.h"
#include "he/core/string_view.h"
#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    enum class TomlError : uint8_t
    {
        None,           ///< No error.
        Cancelled,      ///< The read was cancelled by the handler.
        EmptyFile,      ///< The input TOML data was empty.
        Eof,            ///< The input ended unexpectedly.
        InvalidBom,     ///< The utf-8 Byte Order Mark (BOM) of the file is not valid.
        InvalidToken,   ///< Encountered an unexpected token in the file.
        InvalidValue,   ///< A value was invalid, or out of range.
    };

    // --------------------------------------------------------------------------------------------
    struct TomlResult
    {
        TomlError error{ TomlError::None };
        uint32_t line{ 0 };
        uint32_t column{ 0 };

        [[nodiscard]] explicit operator bool() const { return error == TomlError::None; }
    };

    // --------------------------------------------------------------------------------------------
    class TomlReader
    {
    public:
        class Handler
        {
        public:
            virtual ~Handler() = default;

            // Primative values
            virtual bool Bool(bool value) = 0;
            virtual bool Int(int64_t value) = 0;
            virtual bool Uint(uint64_t value) = 0;
            virtual bool Float(double value) = 0;
            virtual bool String(StringView value) = 0;
            // TODO: Date, Time, DateTime

            // Tables
            virtual bool StartTable() = 0;
            virtual bool Key(StringView name) = 0;
            virtual bool EndTable(uint32_t keyCount) = 0;

            // Arrays
            virtual bool StartArray() = 0;
            virtual bool EndArray(uint32_t length) = 0;
        };

    public:
        TomlResult Read(StringView data, Handler& handler);

    private:
        bool Next();
        bool Next(enum TomlToken expected);
        bool Expect(enum TomlToken expected);
    };

    // --------------------------------------------------------------------------------------------
    class TomlWriter
    {
    public:
        enum class IntFormat : uint8_t
        {
            Decimal,
            Hex,
            Octal,
            Binary,
        };

        enum class FloatFormat : uint8_t
        {
            Fixed,
            Exponent,
        };

    public:
        explicit TomlWriter(String& dst, Allocator& allocator = Allocator::GetDefault()) noexcept
            : m_builder(dst)
            , m_path(allocator)
        {}

        void Clear() { m_builder.Clear(); }
        void Reserve(uint32_t size) { m_builder.Reserve(size); }

        String& Str() { return m_builder.Str(); }
        const String& Str() const { return m_builder.Str(); }

    public:
        void Bool(bool value);
        void Int(int64_t value, IntFormat format = IntFormat::Decimal);
        void Uint(uint64_t value, IntFormat format = IntFormat::Decimal);
        void Float(double value, uint32_t precision = 32, FloatFormat format = FloatFormat::Fixed);
        void String(StringView value, bool multiline = false, bool literal = false);
        // TODO: Date, Time, DateTime

        void Key(StringView name);

        void StartTable(StringView name);
        void EndTable();

        void StartInlineTable();
        void EndInlineTable();

        void StartArray();
        void EndArray();

        void StartInlineArray();
        void EndInlineArray();

    private:
        void InlineArrayComma();
        void InlineTableComma();

    private:
        StringBuilder m_builder;
        he::String m_path;

        uint16_t m_tableCount{ 0 };
        uint16_t m_inlineTableCount{ 0 };
        bool m_firstInlineTableKey{ true };

        uint16_t m_arrayCount{ 0 };
        uint16_t m_inlineArrayCount{ 0 };
        bool m_firstInlineArrayItem{ true };
    };
}
