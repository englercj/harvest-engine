// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    template <typename T>
    struct Limits;

    template <Integral T>
    struct Limits<T>
    {
        using Type = T;

        static constexpr bool IsSigned = T(-1) < T(0);
        static constexpr uint32_t Bits = static_cast<uint32_t>(sizeof(T) * HE_CHAR_BIT);
        static constexpr uint32_t SignBits = IsSigned ? 1 : 0;
        static constexpr uint32_t ValueBits = static_cast<uint32_t>(Bits - SignBits);

        static constexpr uint32_t Digits = ValueBits;
        static constexpr uint32_t Digits10 = static_cast<uint32_t>(Digits * 3 / 10);

        HE_PUSH_WARNINGS();
        HE_DISABLE_MSVC_WARNING(4293);
        HE_DISABLE_MSVC_WARNING(4310);
        static constexpr T Min = IsSigned ? T(T(1) << Digits) : T(0);
        static constexpr T Max = IsSigned ? T(T(~0) ^ Min) : T(~0);
        HE_POP_WARNINGS();
    };

    template <>
    struct Limits<bool>
    {
        using Type = bool;
        static_assert(sizeof(Type) == 1, "Unsupported bool size");

        static constexpr bool IsSigned = false;
        static constexpr uint32_t Bits = (sizeof(bool) * 8);
        static constexpr uint32_t SignBits = 0;
        static constexpr uint32_t ValueBits = 8;

        static constexpr uint32_t Digits = 1;
        static constexpr uint32_t Digits10 = 0;

        static constexpr bool Min = false;
        static constexpr bool Max = true;
    };

    /// Floating point precision representation format.
    enum class FloatFormat
    {
        Unknown,            ///< Unknown or unsupported precision
        Binary32,           ///< IEEE-754 single-precision format (32-bit)
        Binary64,           ///< IEEE-754 double-precision format (64-bit)
        Binary128,          ///< IEEE-754 quadruple-precision format (128-bit)
        DoubleDouble128,    ///< sum of two double-precision format (128-bit)
        Extended80,         ///< x86 extended precision format (80-bit)
    };

    template <>
    struct Limits<float>
    {
        using Type = float;
        static_assert(sizeof(Type) == 4, "Unsupported float size");

        static constexpr FloatFormat Format = FloatFormat::Binary32;

    #if defined(__FLT_MANT_DIG__)
        static constexpr uint32_t Digits = __FLT_MANT_DIG__;
    #else
        static constexpr uint32_t Digits = 24;
    #endif

    #if defined(__FLT_DIG__)
        static constexpr uint32_t Digits10 = __FLT_DIG__;
    #else
        static constexpr uint32_t Digits10 = 6;
    #endif

        static constexpr bool IsSigned = true;
        static constexpr bool HasImplicitBit = true;
        static constexpr uint32_t Bits = 32;
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ExponentBits = 8;
        static constexpr uint32_t SignificandBits = Digits - 1;
        static_assert(SignBits + ExponentBits + SignificandBits == Bits);

    #if defined(__FLT_MAX_EXP__)
        static constexpr uint32_t MaxExponent = __FLT_MAX_EXP__;
    #else
        static constexpr uint32_t MaxExponent = 1u << (ExponentBits - 1);
    #endif

    #if defined(__FLT_MIN_EXP__)
        static constexpr int32_t MinExponent = __FLT_MIN_EXP__;
    #else
        static constexpr int32_t MinExponent = (1 - MaxExponent) + 2;
    #endif

    #if defined(__FLT_MAX__)
        static constexpr float Max = __FLT_MAX__;
    #else
        static constexpr float Max = 3.402823466e+38f;
    #endif

        static constexpr float Min = -Max;

    #if defined(__FLT_MIN__)
        static constexpr float MinPos = __FLT_MIN__;
    #else
        static constexpr float MinPos = 1.175494351e-38f;
    #endif

    #if defined(__FLT_EPSILON__)
        static constexpr float Epsilon = __FLT_EPSILON__;
    #else
        static constexpr float Epsilon = 1.192092896e-07f;
    #endif

    #if defined(__FLT_DENORM_MIN__)
        static constexpr float DenormMin = __FLT_DENORM_MIN__;
    #else
        static constexpr float DenormMin = 1.401298464e-45f;
    #endif

        static constexpr float Infinity = __builtin_huge_valf();
        static constexpr float NaN = __builtin_nanf("0");
        static constexpr float SignalingNaN = __builtin_nansf("1");
    };

    template <>
    struct Limits<double>
    {
        using Type = double;
        static_assert(sizeof(Type) == 8, "Unsupported double size");

        static constexpr FloatFormat Format = FloatFormat::Binary64;

    #if defined(__DBL_MANT_DIG__)
        static constexpr uint32_t Digits = __DBL_MANT_DIG__;
    #else
        static constexpr uint32_t Digits = 53;
    #endif

    #if defined(__DBL_DIG__)
        static constexpr uint32_t Digits10 = __DBL_DIG__;
    #else
        static constexpr uint32_t Digits10 = 15;
    #endif

        static constexpr bool IsSigned = true;
        static constexpr bool HasImplicitBit = true;
        static constexpr uint32_t Bits = 64;
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ExponentBits = 11;
        static constexpr uint32_t SignificandBits = Digits - 1;
        static_assert(SignBits + ExponentBits + SignificandBits == Bits);

    #if defined(__DBL_MAX_EXP__)
        static constexpr uint32_t MaxExponent = __DBL_MAX_EXP__;
    #else
        static constexpr uint32_t MaxExponent = 1u << (ExponentBits - 1);
    #endif

    #if defined(__DBL_MIN_EXP__)
        static constexpr int32_t MinExponent = __DBL_MIN_EXP__;
    #else
        static constexpr int32_t MinExponent = (1 - MaxExponent) + 2;
    #endif

    #if defined(__DBL_MAX__)
        static constexpr double Max = __DBL_MAX__;
    #else
        static constexpr double Max = 1.7976931348623158e+308;
    #endif

        static constexpr double Min = -Max;

    #if defined(__DBL_MIN__)
        static constexpr double MinPos = __DBL_MIN__;
    #else
        static constexpr double MinPos = 2.2250738585072014e-308;
    #endif

    #if defined(__DBL_EPSILON__)
        static constexpr double Epsilon = __DBL_EPSILON__;
    #else
        static constexpr double Epsilon = 2.2204460492503131e-016;
    #endif

    #if defined(__DBL_DENORM_MIN__)
        static constexpr double DenormMin = __DBL_DENORM_MIN__;
    #else
        static constexpr double DenormMin = 4.9406564584124654e-324;
    #endif

        static constexpr double Infinity = __builtin_huge_val();
        static constexpr double NaN = __builtin_nan("0");
        static constexpr double SignalingNaN = __builtin_nans("1");
    };

    template <>
    struct Limits<long double>
    {
        using Type = long double;
    #if defined(__LDBL_MANT_DIG__)
        static_assert(sizeof(Type) == 16, "Unsupported long double size");
    #else
        static_assert(sizeof(Type) == 8, "Unsupported long double size");
    #endif

    #if defined(__LDBL_MANT_DIG__)
        static constexpr uint32_t Digits = __LDBL_MANT_DIG__;
    #else
        static constexpr uint32_t Digits = 53;
    #endif

    #if defined(__LDBL_DIG__)
        static constexpr uint32_t Digits10 = __LDBL_DIG__;
    #else
        static constexpr uint32_t Digits10 = 15;
    #endif

        static constexpr FloatFormat Format =
            Digits == 106 ? FloatFormat::DoubleDouble128
            : Digits == 113 ? FloatFormat::Binary128
            : Digits == 64 ? FloatFormat::Extended80
            : FloatFormat::Binary64;

        static constexpr bool IsSigned = true;
        static constexpr bool HasImplicitBit = Format != FloatFormat::Extended80; // 80-bit long double has a 64-bit significand and no implicit bit
        static constexpr uint32_t Bits = Format == FloatFormat::Extended80 ? 80 : (sizeof(long double) * HE_CHAR_BIT);
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ExponentBits = Format == FloatFormat::Extended80 || Format == FloatFormat::Binary128 ? 15 : 11;
        static constexpr uint32_t SignificandBits = Digits - static_cast<uint32_t>(HasImplicitBit);
        static_assert(SignBits + ExponentBits + SignificandBits == Bits);

    #if defined(__LDBL_MAX_EXP__)
        static constexpr uint32_t MaxExponent = __LDBL_MAX_EXP__;
    #else
        static constexpr uint32_t MaxExponent = 1u << (ExponentBits - 1);
    #endif

    #if defined(__LDBL_MIN_EXP__)
        static constexpr int32_t MinExponent = __LDBL_MIN_EXP__;
    #else
        static constexpr int32_t MinExponent = (1 - MaxExponent) + 2;
    #endif

    #if defined(__LDBL_MAX__)
        static constexpr long double Max = __LDBL_MAX__;
    #else
        static constexpr long double Max = 1.7976931348623158e+308;
    #endif

        static constexpr long double Min = -Max;

    #if defined(__LDBL_MIN__)
        static constexpr long double MinPos = __LDBL_MIN__;
    #else
        static constexpr long double MinPos = 2.2250738585072014e-308;
    #endif

    #if defined(__LDBL_EPSILON__)
        static constexpr long double Epsilon = __LDBL_EPSILON__;
    #else
        static constexpr long double Epsilon = 2.2204460492503131e-016;
    #endif

    #if defined(__LDBL_DENORM_MIN__)
        static constexpr double DenormMin = __LDBL_DENORM_MIN__;
    #else
        static constexpr double DenormMin = 4.9406564584124654e-324;
    #endif

    #if HE_HAS_BUILTIN(__builtin_huge_vall)
        static constexpr long double Infinity = __builtin_huge_vall();
    #else
        static constexpr long double Infinity = __builtin_huge_val();
    #endif

    #if HE_HAS_BUILTIN(__builtin_nanl)
        static constexpr long double NaN = __builtin_nanl("0");
    #else
        static constexpr long double NaN = __builtin_nan("0");
    #endif

    #if HE_HAS_BUILTIN(__builtin_nansl)
        static constexpr long double SignalingNaN = __builtin_nansl("1");
    #else
        static constexpr long double SignalingNaN = __builtin_nans("1");
    #endif
    };
}
