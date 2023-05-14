// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/clock.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/string_writer.h"
#include "he/core/types.h"

namespace he
{
    enum class TomlIntFormat : uint8_t
    {
        Decimal,
        Hex,
        Octal,
        Binary,
    };

    enum class TomlFloatFormat : uint8_t
    {
        Fixed,
        Exponent,
        General,
    };

    class TomlWriter
    {
    public:
        explicit TomlWriter(String& dst, Allocator& allocator = Allocator::GetDefault()) noexcept
            : m_writer(dst)
            , m_path(allocator)
        {}

        void Clear() { m_writer.Clear(); }
        void Reserve(uint32_t size) { m_writer.Reserve(size); }

    public:
        void Bool(bool value);
        void Int(int64_t value, TomlIntFormat format = TomlIntFormat::Decimal);
        void Uint(uint64_t value, TomlIntFormat format = TomlIntFormat::Decimal);
        void Float(double value, int32_t precision = -1, TomlFloatFormat format = TomlFloatFormat::General);
        void String(StringView value, bool multiline = false, bool literal = false);
        void DateTime(SystemTime value, bool utc = true);
        void Time(Duration value);

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
        StringWriter m_writer;
        he::String m_path;

        uint16_t m_tableCount{ 0 };
        uint16_t m_inlineTableCount{ 0 };
        bool m_firstInlineTableKey{ true };

        uint16_t m_arrayCount{ 0 };
        uint16_t m_inlineArrayCount{ 0 };
        bool m_firstInlineArrayItem{ true };
    };
}
