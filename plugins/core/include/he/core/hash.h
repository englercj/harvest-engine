// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    /// Creates a combined hash value from two existing hash values. This creates a much better
    /// hash than adding or xor'ing the values does.
    ///
    /// \param[in] a The first hash value to combine.
    /// \param[in] b The second hash value to combine.
    /// \return The resulting combined hash value.
    constexpr uint16_t CombineHash16(uint16_t a, uint16_t b) noexcept
    {
        return a ^ (b + 0x9e37u + (a << 3) + (a >> 1));
    }

    /// \copydoc CombineHash16
    constexpr uint32_t CombineHash32(uint32_t a, uint32_t b) noexcept
    {
        return a ^ (b + 0x9e3779b9u + (a << 6) + (a >> 2));
    }

    /// \copydoc CombineHash16
    constexpr uint64_t CombineHash64(uint64_t a, uint64_t b) noexcept
    {
        return a ^ (b + 0x9e3779b97f4a7c15ull + (a << 12) + (a >> 4));
    }

    /// A bit mixer that creates an avalanche effect. Based on Murmur3's mixing function.
    ///
    /// \param[in] value The value to mix.
    /// \return The mixed result.
    constexpr uint32_t Mix32(uint32_t value) noexcept
    {
        value ^= value >> 16;
        value *= 0x85ebca6bu;
        value ^= value >> 13;
        value *= 0xc2b2ae35u;
        value ^= value >> 16;
        return value;
    }

    /// \copydoc Mix32
    constexpr uint64_t Mix64(uint64_t value) noexcept
    {
        value ^= value >> 33;
        value *= 0xff51afd7ed558ccdull;
        value ^= value >> 33;
        value *= 0xc4ceb9fe1a85ec53ull;
        value ^= value >> 33;
        return value;
    }

    // --------------------------------------------------------------------------------------------
    // Streaming hash implementation that allows for incremental updates and a final value.
    template <typename Algo>
    class Hash
    {
    public:
        using ValueType = typename Algo::ValueType;

    public:
        explicit Hash(ValueType seed = Algo::DefaultSeed) noexcept : m_state(seed) {}

        void Reset(ValueType seed = Algo::DefaultSeed) { m_state = seed; }

        template <typename T> requires(Arithmetic<T> || Enum<T>)
        Hash& Update(const T& value) { m_state = Algo::Mem(&value, sizeof(T), m_state); return *this; }

        template <ArithmeticRange R>
        Hash& Update(const R& range) { m_state = Algo::Mem(range.Data(), range.Size(), m_state); return *this; }

        Hash& Update(const char* str) { m_state = Algo::Mem(str, StrLen(str), m_state); return *this; }

        Hash& Update(const void* data, uint32_t len) { m_state = Algo::Mem(data, len, m_state); return *this; }

        ValueType Finalize() { return m_state; }

    private:
        ValueType m_state;
    };

    // --------------------------------------------------------------------------------------------
    // Fowler–Noll–Vo (FNV-1a) 32-bit non-cryptographic hash
    struct FNV32
    {
        using ValueType = uint32_t;

        static constexpr uint32_t DefaultSeed{ 0x811c9dc5 };
        static constexpr uint32_t Prime{ 0x1000193 };

        static uint32_t Mem(const void* data, uint32_t len, uint32_t seed = DefaultSeed);
        static constexpr uint32_t String(const char* str, uint32_t seed = DefaultSeed);
        static constexpr uint32_t String(StringView view, uint32_t seed = DefaultSeed);
    };

    // --------------------------------------------------------------------------------------------
    // Fowler–Noll–Vo (FNV-1a) 64-bit non-cryptographic hash
    struct FNV64
    {
        using ValueType = uint64_t;

        static constexpr uint64_t DefaultSeed{ 0xcbf29ce484222325 };
        static constexpr uint64_t Prime{ 0x100000001b3 };

        static uint64_t Mem(const void* data, uint32_t len, uint64_t seed = DefaultSeed);
        static constexpr uint64_t String(const char* str, uint64_t seed = DefaultSeed);
        static constexpr uint64_t String(StringView view, uint64_t seed = DefaultSeed);
    };

    // --------------------------------------------------------------------------------------------
    // CRC-32C (Castagnoli) 32-bit non-cryptographic cyclic redundancy check
    struct CRC32C
    {
        using ValueType = uint32_t;

        static constexpr uint32_t DefaultSeed{ 0 };

        static uint32_t Mem(const void* data, uint32_t len, uint32_t seed = DefaultSeed);
    };

    // --------------------------------------------------------------------------------------------
    // WyHash 64-bit non-cryptographic avalanching hash by Wang Yi
    // See: https://github.com/wangyi-fudan/wyhash
    struct WyHash
    {
        using ValueType = uint64_t;

        static constexpr uint32_t DefaultSeed{ 0 };

        static uint64_t Mem(const void* data, uint32_t len, uint64_t seed = DefaultSeed);
    };

    // --------------------------------------------------------------------------------------------
    // Secure Hash Algorithm 1 (SHA1) 160-bit cryptographic hash
    struct SHA1
    {
        struct Value
        {
            uint8_t bytes[20];
            bool operator==(const Value& x);
            bool operator!=(const Value& x);
        };

        using ValueType = Value;

        static Value Mem(const void* data, uint32_t len, uint64_t = 0);
    };

    template <>
    class Hash<SHA1>
    {
    public:
        using ValueType = typename SHA1::ValueType;
        static constexpr uint32_t BlockSize = 64;

    public:
        explicit Hash() noexcept;

        void Reset();

        template <typename T> requires(Arithmetic<T> || Enum<T>)
        Hash& Update(const T& value) { Update(&value, sizeof(T)); return *this; }

        template <ArithmeticRange R>
        Hash& Update(const R& range) { Update(range.Data(), range.Size()); return *this; }

        Hash& Update(const char* str) { Update(str, StrLen(str)); return *this; }

        Hash& Update(const void* data, uint32_t len);

        ValueType Finalize();

    private:
        void Process_SW(const uint8_t* data, uint32_t len);
        void Process_SSE41(const uint8_t* data, uint32_t len);
        void Process_NEON(const uint8_t* data, uint32_t len);

    private:
        uint64_t m_length;
        uint32_t m_state[5];
        uint32_t m_bufLen;
        uint8_t m_buf[64];
    };

    // --------------------------------------------------------------------------------------------
    // Secure Hash Algorithm 2 (SHA256) 256-bit cryptographic hash
    struct SHA256
    {
        struct Value
        {
            uint8_t bytes[32];
            bool operator==(const Value& x);
            bool operator!=(const Value& x);
        };

        using ValueType = Value;

        static Value Mem(const void* data, uint32_t len, uint64_t = 0);
    };

    template <>
    class Hash<SHA256>
    {
    public:
        using ValueType = typename SHA1::ValueType;
        static constexpr uint32_t BlockSize = 64;

    public:
        explicit Hash() noexcept;

        void Reset();

        template <typename T> requires(Arithmetic<T> || Enum<T>)
        Hash& Update(const T& value) { Update(&value, sizeof(T)); return *this; }

        template <ArithmeticRange R>
        Hash& Update(const R& range) { Update(range.Data(), range.Size()); return *this; }

        Hash& Update(const char* str) { Update(str, StrLen(str)); return *this; }

        Hash& Update(const void* data, uint32_t len);

        ValueType Finalize();

    private:
        void Process_SW(const uint8_t* data, uint32_t len);
        void Process_SSE41(const uint8_t* data, uint32_t len);
        void Process_NEON(const uint8_t* data, uint32_t len);

    private:
        uint64_t m_length;
        uint32_t m_state[5];
        uint32_t m_bufLen;
        uint8_t m_buf[64];
    };

    // --------------------------------------------------------------------------------------------
    /// A functor for calculating a uint64_t hash of a value. It is expected to overload operator()
    /// and return a uint64_t value. The recommended prototype of this function is:
    /// ```
    /// [[nodiscard]] uint64_t operator()(const T& value) const noexcept;
    /// ```
    ///
    /// The resulting value is expected to be uniformly distributed and have an avalanche effect,
    /// where small changes to the input should result in very different outputs. When implementing
    /// specializations of this functor, keep in mind that the \ref Mix64 function and \ref WyHash
    /// algorithm can be helpful to satisfy these requirements. Also note that \ref FNV32 and
    /// \ref FNV64 do *not* exhibit avalanche behavior, and therefore are not suitable alone.
    ///
    /// Rather than specializing this functor, an object may instead define a constant non-static
    /// member function called `HashCode` which takes no arguments and returns a `uint64_t`.  The
    /// same expectations about the return value that were mentioned above apply to this function
    /// as well. The recommended prototype of such a function is:
    /// ```
    /// [[nodiscard]] uint64_t HashCode() const noexcept;
    /// ```
    template <typename T> struct Hasher;

    /// Hasher specialization for boolean values.
    template <>
    struct Hasher<bool>
    {
        [[nodiscard]] constexpr uint64_t operator()(bool value) const noexcept
        {
            return value ? static_cast<uint64_t>(-1) : 0;
        }
    };

    /// Hasher specialization for integral and enum values.
    template <typename T> requires(IsIntegral<T> || IsEnum<T>)
    struct Hasher<T>
    {
        [[nodiscard]] constexpr uint64_t operator()(const T& value) const noexcept
        {
            static_assert(sizeof(T) <= sizeof(uint64_t));
            const uint64_t v = static_cast<uint64_t>(value);
            return Mix64(v);
        }
    };

    /// Hasher specialization for floating point values.
    template <typename T> requires(IsFloatingPoint<T>)
    struct Hasher<T>
    {
        [[nodiscard]] uint64_t operator()(const T& value) const noexcept
        {
            // Ensure that 0 and -0 result in the same hash value
            if (value == T{})
                return 0;

            static_assert(sizeof(T) <= sizeof(uint64_t));
            uint64_t v = 0;
            MemCopy(&v, &value, sizeof(T));
            return Mix64(v);
        }
    };

    /// Hasher specialization for pointer values.
    template <typename T>
    struct Hasher<T*>
    {
        [[nodiscard]] constexpr uint64_t operator()(T* value) const noexcept
        {
            const uintptr_t v = BitCast<uintptr_t>(value);
            return Hasher<uintptr_t>()(v);
        }
    };

    /// Concept for an object that has a constant non-static member function `HashCode` which
    /// takes no arguments and returns `uint64_t`.
    template <typename T>
    concept HasHashCode = requires(const T& t)
    {
        { t.HashCode() } -> SameAs<uint64_t>;
    };

    /// Hasher specialization for objects which define a `HashCode()` function.
    template <HasHashCode T>
    struct Hasher<T>
    {
        constexpr uint64_t operator()(const T& value) const noexcept
        {
            return value.HashCode();
        }
    };

    /// \internal
    template <typename T>
    struct _IsHashable : FalseType {};

    /// \internal
    template <typename T> requires(IsSame<decltype(DeclVal<Hasher<T>>()(DeclVal<const T&>())), uint64_t>)
    struct _IsHashable<T> : TrueType {};

    /// Concept for a type that can be hashed using a \ref Hasher specialization.
    template <typename T>
    concept Hashable = _IsHashable<T>::Value;

    /// Helper function to get the hash code of a value. This function dispatches to the proper
    /// \ref Hasher specialization for the type.
    ///
    /// \param[in] value The value to get the hash code of.
    /// \return The hash code for the value.
    template <Hashable T>
    constexpr HE_FORCE_INLINE uint64_t GetHashCode(const T& value) noexcept
    {
        return Hasher<T>()(value);
    }
}

#include "he/core/inline/hash.inl"

// TODO: remove this when we stop using std containers
namespace std
{
    template <typename> struct hash;

    template <he::Hashable T>
    struct hash<T>
    {
        size_t operator()(const T& value) const
        {
            return he::GetHashCode(value);
        }
    };
}
