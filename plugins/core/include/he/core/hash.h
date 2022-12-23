// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/memory_ops.h"
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
    // Fowler–Noll–Vo (FNV-1a) 32-bit non-cryptographic hash
    class FNV32
    {
    public:
        using ValueType = uint32_t;
        static constexpr ValueType Seed{ 0x811c9dc5 };

        static constexpr ValueType HashString(const char* str, ValueType seed = Seed);
        static constexpr ValueType HashStringN(const char* str, uint32_t len, ValueType seed = Seed);

        static ValueType HashData(const void* data, uint32_t len, ValueType seed = Seed);

        template <Arithmetic T>
        static ValueType HashScalar(const T& obj, ValueType seed = Seed);

    public:
        FNV32(ValueType seed = Seed);

        template <Arithmetic T>
        FNV32& Scalar(const T& obj);

        FNV32& String(const char* str);
        FNV32& Data(const void* data, uint32_t len);

        FNV32& Reset(ValueType seed = Seed);

        ValueType Done();

    private:
        ValueType m_state;
    };

    // --------------------------------------------------------------------------------------------
    // Fowler–Noll–Vo (FNV-1a) 64-bit non-cryptographic hash
    class FNV64
    {
    public:
        using ValueType = uint64_t;
        static constexpr ValueType Seed{ 0xcbf29ce484222325 };

        static constexpr ValueType HashString(const char* str, ValueType seed = Seed);
        static constexpr ValueType HashStringN(const char* str, uint32_t len, ValueType seed = Seed);

        static ValueType HashData(const void* data, uint32_t len, ValueType seed = Seed);

        template <Arithmetic T>
        static ValueType HashScalar(const T& obj, ValueType seed = Seed);

    public:
        FNV64(ValueType seed = Seed);

        template <Arithmetic T>
        FNV64& Scalar(const T& obj);

        FNV64& String(const char* str);
        FNV64& Data(const void* data, uint32_t len);

        FNV64& Reset(ValueType seed = Seed);

        ValueType Done();

    private:
        ValueType m_state;
    };

    // --------------------------------------------------------------------------------------------
    // CRC-32C (Castagnoli) 32-bit non-cryptographic cyclic redundancy check
    class CRC32C
    {
    public:
        using ValueType = uint32_t;
        static constexpr ValueType Seed{ 0 };

        static ValueType HashString(const char* str, ValueType seed = Seed);
        static ValueType HashData(const void* data, uint32_t len, ValueType seed = Seed);

        template <Arithmetic T>
        static ValueType HashScalar(const T& obj, ValueType seed = Seed);

    public:
        CRC32C(ValueType seed = Seed);

        template <Arithmetic T>
        CRC32C& Scalar(const T& obj);

        CRC32C& String(const char* str);
        CRC32C& Data(const void* data, uint32_t len);

        CRC32C& Reset(ValueType seed = Seed);

        ValueType Done();

    private:
        ValueType m_state;
    };

    // --------------------------------------------------------------------------------------------
    // WyHash 64-bit non-cryptographic avalanching hash by Wang Yi
    // See: https://github.com/wangyi-fudan/wyhash
    class WyHash
    {
    public:
        using ValueType = uint64_t;
        static constexpr ValueType Seed{ 0 };

        static ValueType HashString(const char* str, ValueType seed = Seed);
        static ValueType HashData(const void* data, uint32_t len, ValueType seed = Seed);

        template <Arithmetic T>
        static ValueType HashScalar(const T& obj, ValueType seed = Seed);

    public:
        WyHash(ValueType seed = Seed);

        template <Arithmetic T>
        WyHash& Scalar(const T& obj);

        WyHash& String(const char* str);
        WyHash& Data(const void* data, uint32_t len);

        WyHash& Reset(ValueType seed = Seed);

        ValueType Done();

    private:
        ValueType m_state;
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
    /// \ref FNV64 do *not* exhibit avalanche behavior, and therefore are not suitable.
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
    template <typename T> requires(std::is_integral_v<T> || std::is_enum_v<T>)
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
    template <typename T> requires(std::is_floating_point_v<T>)
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
        { t.HashCode() } -> Exactly<uint64_t>;
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
    template <typename T, typename = std::void_t<>>
    struct _IsHashable : std::false_type {};

    /// \internal
    template <typename T>
    struct _IsHashable<T, std::void_t<decltype(std::declval<Hasher<T>>()(std::declval<T>()))>> : std::true_type {};

    /// Concept for a type that can be hashed using a \ref Hasher specialization.
    template <typename T>
    concept Hashable = _IsHashable<T>::value;

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
