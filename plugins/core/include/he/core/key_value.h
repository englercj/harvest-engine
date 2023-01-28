// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/concepts.h"
#include "he/core/enum_ops.h"
#include "he/core/fmt.h"
#include "he/core/type_traits.h"
#include "he/core/string.h"
#include "he/core/utils.h"

/// \def HE_MSG_KEY
/// Defines the quoted string to use as the key for the special "message" pair that is
/// created in the HE_MSG macro. By default this is defined as "message".
#if !defined(HE_MSG_KEY)
    #define HE_MSG_KEY "message"
#endif

/// Macro that simplifies the construction of a key-value pair.
/// This is primarily used with the logging and assertion libraries.
///
/// \param k The unquoted name of the key. By convention these are snake_case.
/// \param v The value of the pair, which can any arithmetic type or anything convertable to a
///     string via \ref Format. Make sure to include the "*_fmt.h" header for the type you use.
///     You can also specify a format string followed by arguments to format. If there are no
///     format arguments a string will be used as-is, without formatting.
/// \param ... The format arguments if `v` is a format string specifier.
#define HE_KV(k, v, ...) (::he::KeyValue{ #k, (v), ##__VA_ARGS__ })

/// Shortcut macro for creating a key-value pair that represents a variable. The stringified name
/// of the variable is used as the key.
///
/// \param x The variable to create a key-value pair from.
#define HE_VAL(x) (::he::KeyValue{ #x, (x) })

/// Shortcut macro for creating a key-value pair that represents a string message.
/// This generates a key-value pair using the expansion of \see HE_MSG_KEY as the key.
///
/// \param fmt The format string. If there are no format arguments the string is used as-is,
///     without formatting.
/// \param ... The format arguments.
#define HE_MSG(fmt, ...) (::he::KeyValue{ HE_MSG_KEY, (fmt), ##__VA_ARGS__ })

namespace he
{
    /// A key-value pair where keys are strings and values are stored in a tagged union.
    /// Keys are not copied, only the pointer is stored.
    ///
    /// \note Prefer using the HE_KV macro rather than creating this structure directly.
    class KeyValue
    {
    public:
        enum class ValueKind
        {
            Bool,
            Enum,
            Int,
            Uint,
            Double,
            String,
        };

    public:
        KeyValue(const char* k, bool v) noexcept : m_key(k), m_kind(ValueKind::Bool), m_value{ .b = v } {}
        KeyValue(const char* k, signed char v) noexcept : m_key(k), m_kind(ValueKind::Int), m_value{ .i = v } {}
        KeyValue(const char* k, signed short v) noexcept : m_key(k), m_kind(ValueKind::Int), m_value{ .i = v } {}
        KeyValue(const char* k, signed int v) noexcept : m_key(k), m_kind(ValueKind::Int), m_value{ .i = v } {}
        KeyValue(const char* k, signed long v) noexcept : m_key(k), m_kind(ValueKind::Int), m_value{ .i = v } {}
        KeyValue(const char* k, signed long long v) noexcept : m_key(k), m_kind(ValueKind::Int), m_value{ .i = v } {}
        KeyValue(const char* k, unsigned char v) noexcept : m_key(k), m_kind(ValueKind::Uint), m_value{ .u = v } {}
        KeyValue(const char* k, unsigned short v) noexcept : m_key(k), m_kind(ValueKind::Uint), m_value{ .u = v } {}
        KeyValue(const char* k, unsigned int v) noexcept : m_key(k), m_kind(ValueKind::Uint), m_value{ .u = v } {}
        KeyValue(const char* k, unsigned long v) noexcept : m_key(k), m_kind(ValueKind::Uint), m_value{ .u = v } {}
        KeyValue(const char* k, unsigned long long v) noexcept : m_key(k), m_kind(ValueKind::Uint), m_value{ .u = v } {}
        KeyValue(const char* k, float v) noexcept : m_key(k), m_kind(ValueKind::Double), m_value{ .d = v } {}
        KeyValue(const char* k, double v) noexcept : m_key(k), m_kind(ValueKind::Double), m_value{ .d = v } {}

        template <Enum T>
        constexpr KeyValue(const char* k, T v) noexcept
            : m_key(k)
            , m_kind(ValueKind::Enum)
        {
            m_value.e.value = static_cast<uint64_t>(v);
            m_value.e.toString = [](uint64_t val) { return AsString(static_cast<T>(val)); };
        }

        KeyValue(const char* k, const char* v) noexcept
            : m_key(k)
            , m_kind(ValueKind::String)
        {
            m_value.s = v;
        }

        template <size_t N>
        KeyValue(const char* k, const char (&v)[N]) noexcept
            : m_key(k)
            , m_kind(ValueKind::String)
        {
            m_value.s.Assign(v, N);
        }

        template <typename T> requires(!IsEnum<T> && ContiguousRange<T, const char>)
        KeyValue(const char* k, const T& v) noexcept
            : m_key(k)
            , m_kind(ValueKind::String)
        {
            m_value.s = v;
        }

        template <typename... Args>
        KeyValue(const char* k, FmtString<Args...> fmt, Args&&... args) noexcept
            : m_key(k)
            , m_kind(ValueKind::String)
        {
            FormatTo(m_value.s, fmt, Forward<Args>(args)...);
        }

        template <typename T> requires(!IsEnum<T> && !ContiguousRange<T, const char>)
        KeyValue(const char* k, const T& v) noexcept
            : m_key(k)
            , m_kind(ValueKind::String)
        {
            FormatTo(m_value.s, "{}", v);
        }

        KeyValue(const KeyValue& x) noexcept { *this = x; }
        KeyValue& operator=(const KeyValue& x) noexcept
        {
            m_key = x.m_key;
            m_kind = x.m_kind;
            switch (m_kind)
            {
                case ValueKind::Bool: m_value.b = x.m_value.b; break;
                case ValueKind::Enum: m_value.e = x.m_value.e; break;
                case ValueKind::Int: m_value.i = x.m_value.i; break;
                case ValueKind::Uint: m_value.u = x.m_value.u; break;
                case ValueKind::Double: m_value.d = x.m_value.d; break;
                case ValueKind::String: m_value.s = x.m_value.s; break;
            }
            return *this;
        }

        KeyValue(KeyValue&& x) noexcept { *this = Move(x); }
        KeyValue& operator=(KeyValue&& x) noexcept
        {
            m_key = x.m_key;
            m_kind = x.m_kind;
            switch (m_kind)
            {
                case ValueKind::Bool: m_value.b = x.m_value.b; break;
                case ValueKind::Enum: m_value.e = x.m_value.e; break;
                case ValueKind::Int: m_value.i = x.m_value.i; break;
                case ValueKind::Uint: m_value.u = x.m_value.u; break;
                case ValueKind::Double: m_value.d = x.m_value.d; break;
                case ValueKind::String: m_value.s = Move(x.m_value.s); break;
            }
            return *this;
        }

    public:
        const char* Key() const { return m_key; }
        ValueKind Kind() const { return m_kind; }

        bool GetBool() const;
        int64_t GetInt() const;
        uint64_t GetUint() const;
        double GetDouble() const;
        const String& GetString() const;

        uint64_t GetEnumValue() const;
        template <Enum T> T GetEnum() const { return static_cast<T>(GetEnumValue()); }
        const char* GetEnumString() const;

    private:
        const char* m_key{ nullptr };
        ValueKind m_kind{ ValueKind::Bool };

        struct
        {
            union
            {
                bool b{ false };
                int64_t i;
                uint64_t u;
                double d;

                struct
                {
                    uint64_t value;
                    const char* (*toString)(uint64_t value);
                } e;
            };
            String s{ Allocator::GetDefault() };
        } m_value;
    };
}
