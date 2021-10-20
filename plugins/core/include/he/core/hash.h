// Copyright Chad Engler

#pragma once

#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
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
