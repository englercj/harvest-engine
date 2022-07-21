// Copyright Chad Engler

#pragma once

#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    inline uint16_t CombineHash(uint16_t a, uint16_t b)
    {
        constexpr uint16_t C = 0x9e37u;
        return a ^ (b + C + (a << 3) + (a >> 1));
    }

    inline uint32_t CombineHash(uint32_t a, uint32_t b)
    {
        constexpr uint32_t C = 0x9e3779b9u;
        return a ^ (b + C + (a << 6) + (a >> 2));
    }

    inline uint64_t CombineHash(uint64_t a, uint64_t b)
    {
        constexpr uint64_t C = 0x9e3779b97f4a7c15ull;
        return a ^ (b + C + (a << 12) + (a >> 4));
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

        FNV32& String(const char* str);
        FNV32& Data(const void* data, uint32_t len);

        template <Arithmetic T>
        FNV32& Scalar(const T& obj);

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

        FNV64& String(const char* str);
        FNV64& Data(const void* data, uint32_t len);

        template <Arithmetic T>
        FNV64& Scalar(const T& obj);

        FNV64& Reset(ValueType seed = Seed);

        ValueType Done();

    private:
        ValueType m_state;
    };
}

#include "he/core/inline/hash.inl"
