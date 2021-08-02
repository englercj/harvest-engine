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
        using ValueType = uint32;
        static constexpr ValueType Seed{ 0x811c9dc5ul };

        static constexpr ValueType HashString(const char* str, ValueType seed = Seed);
        static constexpr ValueType HashData(const void* data, size_t len, ValueType seed = Seed);

        template <typename T, HE_REQUIRES(IsArithmetic<T>)>
        static constexpr ValueType HashScalar(const T& obj, ValueType seed = Seed);

    public:
        FNV32(ValueType seed = Seed);

        FNV32& String(const char* str);
        FNV32& Data(const void* data, size_t len);

        template <typename T, HE_REQUIRES(IsArithmetic<T>)>
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
        using ValueType = uint64;
        static constexpr ValueType Seed{ 0xcbf29ce484222325ull };

        static constexpr ValueType HashString(const char* str, ValueType seed = Seed);
        static constexpr ValueType HashData(const void* data, size_t len, ValueType seed = Seed);

        template <typename T, HE_REQUIRES(IsArithmetic<T>)>
        static constexpr ValueType HashScalar(const T& obj, ValueType seed = Seed);

    public:
        FNV64(ValueType seed = Seed);

        FNV64& String(const char* str);
        FNV64& Data(const void* data, size_t len);

        template <typename T, HE_REQUIRES(IsArithmetic<T>)>
        FNV64& Scalar(const T& obj);

        FNV64& Reset(ValueType seed = Seed);

        ValueType Done();

    private:
        ValueType m_state;
    };
}

#include "he/core/inline/hash.inl"
