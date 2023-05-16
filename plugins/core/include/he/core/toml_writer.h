// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/clock.h"
#include "he/core/span.h"
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

    enum class TomlStringFormat : uint8_t
    {
        Basic,
        Literal,
    };

    enum class TomlDateTimeFormat : uint8_t
    {
        Utc,
        Local,
    };

    class TomlWriter
    {
    public:
        explicit TomlWriter(String& dst) noexcept
            : m_writer(dst)
        {}

        void Clear() { m_writer.Clear(); }
        void Reserve(uint32_t size) { m_writer.Reserve(size); }

    public:
        void Comment(StringView value);
        void Newline();
        void Write(StringView str);

        void IncreaseIndent();
        void DecreaseIndent();

        // Primitive values
        void Bool(bool value);
        void Int(int64_t value, TomlIntFormat format = TomlIntFormat::Decimal);
        void Uint(uint64_t value, TomlIntFormat format = TomlIntFormat::Decimal);
        void Float(double value, TomlFloatFormat format = TomlFloatFormat::General, int32_t precision = -1);
        void String(StringView value, TomlStringFormat format = TomlStringFormat::Basic);
        void DateTime(SystemTime value, TomlDateTimeFormat format = TomlDateTimeFormat::Utc);
        void Time(Duration value);

        // Tables
        void Table(StringView name, bool isArray = false);
        void Table(Span<StringView> names, bool isArray = false);
        void Key(StringView name);
        void Key(Span<StringView> names);

        // Inline Tables
        void StartInlineTable();
        void EndInlineTable();

        // Arrays
        void StartArray();
        void EndArray();

    private:
        void ArrayComma();
        void InlineTableComma();
        void WriteKeyName(StringView name);
        void WriteKeyNames(Span<StringView> names);

    private:
        static constexpr uint32_t MaxInlineDepth = 128;
        static constexpr uint32_t StatesPerByte = 4;
        static constexpr uint32_t InlineStackBytes = MaxInlineDepth / StatesPerByte;

        enum class InlineKind : uint8_t
        {
            None = 0,
            Array = (1 << 0),
            Table = (1 << 1),
            All = Array | Table,
        };
        void PushInline(InlineKind kind);
        void PopInline(InlineKind kind);

        bool IsIn(InlineKind kind);

    private:
        StringWriter m_writer;

        bool m_firstInlineTableKey{ true };
        bool m_firstArrayItem{ true };

        uint8_t m_inlineStack[InlineStackBytes]{};
        uint32_t m_inlineIndex{ 0 };
    };
}
