// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/concepts.h"
#include "he/core/enum_ops.h"
#include "he/core/fmt.h"
#include "he/core/type_traits.h"
#include "he/core/string.h"
#include "he/core/utils.h"
#include "he/core/variant.h"

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
        struct EnumStorage
        {
            uint64_t value;
            const char* (*toString)(uint64_t value);

            template <Enum T>
            T As() const { return static_cast<T>(value); }

            const char* String() const { return toString(value); }
        };

        using VariantType = Variant<bool, EnumStorage, int64_t, uint64_t, double, String>;

        enum class ValueKind : VariantType::IndexType
        {
            Bool,
            Enum,
            Int,
            Uint,
            Double,
            String,
            Empty,
        };

        template <ValueKind K, VariantType::IndexType I = EnumToValue(K)>
        static consteval IndexConstant<I> AsIndex() { return IndexConstant<I>{}; }

    public:
        KeyValue() noexcept : m_key(""), m_value() {}
        KeyValue(const char* k, bool v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Bool>(), v) {}
        KeyValue(const char* k, char v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Int>(), v) {}
        KeyValue(const char* k, signed char v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Int>(), v) {}
        KeyValue(const char* k, short v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Int>(), v) {}
        KeyValue(const char* k, int v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Int>(), v) {}
        KeyValue(const char* k, long v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Int>(), v) {}
        KeyValue(const char* k, long long v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Int>(), v) {}
        KeyValue(const char* k, unsigned char v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Uint>(), v) {}
        KeyValue(const char* k, unsigned short v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Uint>(), v) {}
        KeyValue(const char* k, unsigned int v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Uint>(), v) {}
        KeyValue(const char* k, unsigned long v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Uint>(), v) {}
        KeyValue(const char* k, unsigned long long v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Uint>(), v) {}
        KeyValue(const char* k, float v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Double>(), v) {}
        KeyValue(const char* k, double v) noexcept : m_key(k), m_value(AsIndex<ValueKind::Double>(), v) {}

        template <Enum T>
        constexpr KeyValue(const char* k, T v) noexcept
            : m_key(k)
            , m_value(AsIndex<ValueKind::Enum>())
        {
            EnumStorage& e = m_value.Get<EnumToValue(ValueKind::Enum)>();
            e.value = static_cast<uint64_t>(v);
            e.toString = [](uint64_t val) { return EnumToString(static_cast<T>(val)); };
        }

        KeyValue(const char* k, const char* v) noexcept
            : m_key(k)
            , m_value(AsIndex<ValueKind::String>(), v)
        {}

        template <uint32_t N>
        KeyValue(const char* k, const char (&v)[N]) noexcept
            : m_key(k)
            , m_value(AsIndex<ValueKind::String>(), v, N)
        {}

        template <typename T> requires(!IsEnum<T> && ContiguousRangeOf<T, const char>)
        KeyValue(const char* k, const T& v) noexcept
            : m_key(k)
            , m_value(AsIndex<ValueKind::String>(), v)
        {}

        template <typename... Args>
        KeyValue(const char* k, FmtString<Args...> fmt, Args&&... args) noexcept
            : m_key(k)
            , m_value(AsIndex<ValueKind::String>())
        {
            he::String& s = m_value.Get<EnumToValue(ValueKind::String)>();
            FormatTo(s, fmt, Forward<Args>(args)...);
        }

        template <typename T> requires(!IsEnum<T> && !ContiguousRangeOf<T, const char>)
        KeyValue(const char* k, const T& v) noexcept
            : m_key(k)
            , m_value(AsIndex<ValueKind::String>())
        {
            he::String& s = m_value.Get<EnumToValue(ValueKind::String)>();
            FormatTo(s, "{}", v);
        }

        KeyValue(const KeyValue& x) noexcept { *this = x; }
        KeyValue& operator=(const KeyValue& x) noexcept
        {
            m_key = x.m_key;
            m_value = x.m_value;
            return *this;
        }

        KeyValue(KeyValue&& x) noexcept { *this = Move(x); }
        KeyValue& operator=(KeyValue&& x) noexcept
        {
            m_key = x.m_key;
            m_value = Move(x.m_value);
            return *this;
        }

    public:
        ValueKind Kind() const { return m_value.IsValid() ? static_cast<ValueKind>(m_value.Index()) : ValueKind::Empty; }
        bool IsEmpty() const { return !m_value.IsValid(); }
        void Clear() { m_value.Clear(); }

        const char* Key() const { return m_key; }

        template <ValueKind K> bool Is() const { return m_value.Index() == EnumToValue(K); }
        bool IsBool() const { return Is<ValueKind::Bool>(); }
        bool IsEnum() const { return Is<ValueKind::Enum>(); }
        bool IsInt() const { return Is<ValueKind::Int>(); }
        bool IsUint() const { return Is<ValueKind::Uint>(); }
        bool IsDouble() const { return Is<ValueKind::Double>(); }
        bool IsString() const { return Is<ValueKind::String>(); }

        template <ValueKind K> decltype(auto) Set() { return m_value.Emplace<EnumToValue(K)>(); }
        void SetBool(bool v) { Set<ValueKind::Bool>() = v; }
        void SetInt(int64_t v) { Set<ValueKind::Int>() = v; }
        void SetUint(uint64_t v) { Set<ValueKind::Uint>() = v; }
        void SetDouble(double v) { Set<ValueKind::Double>() = v; }
        void SetString(StringView v) { Set<ValueKind::String>() = v; }

        template <Enum T>
        void SetEnum(T v)
        {
            EnumStorage& e = Set<ValueKind::Enum>();
            e.value = static_cast<uint64_t>(v);
            e.toString = [](uint64_t val) { return EnumToString(static_cast<T>(val)); };
        }

        template <ValueKind K> decltype(auto) Get() { return m_value.Get<EnumToValue(K)>(); }
        template <ValueKind K> decltype(auto) Get() const { return m_value.Get<EnumToValue(K)>(); }
        bool Bool() const { return Get<ValueKind::Bool>(); }
        const EnumStorage& Enum() const { return Get<ValueKind::Enum>(); }
        int64_t Int() const { return Get<ValueKind::Int>(); }
        uint64_t Uint() const { return Get<ValueKind::Uint>(); }
        double Double() const { return Get<ValueKind::Double>(); }
        const he::String& String() const { return Get<ValueKind::String>(); }

    private:
        const char* m_key;
        VariantType m_value;
    };
}
