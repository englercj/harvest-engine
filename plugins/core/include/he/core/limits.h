// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

// ------------------------------------------------------------------------------------------------
#if defined(__SIZEOF_FLOAT__)
    #define HE_SIZEOF_FLOAT __SIZEOF_FLOAT__
#else
    #define HE_SIZEOF_FLOAT 4
#endif

#if defined(__FLT_MANT_DIG__)
    #define HE_FLT_MANT_DIG __FLT_MANT_DIG__
#else
    #define HE_FLT_MANT_DIG 24
#endif

#if defined(__FLT_DIG__)
    #define HE_FLT_DIG __FLT_DIG__
#else
    #define HE_FLT_DIG 6
#endif

#if defined(__FLT_MAX_EXP__)
    #define HE_FLT_MAX_EXP __FLT_MAX_EXP__
#else
    #define HE_FLT_MAX_EXP 128
#endif

#if defined(__FLT_MIN_EXP__)
    #define HE_FLT_MIN_EXP __FLT_MIN_EXP__
#else
    #define HE_FLT_MIN_EXP (-125)
#endif

#if defined(__FLT_MAX__)
    #define HE_FLT_MAX __FLT_MAX__
#else
    #define HE_FLT_MAX 3.40282346638528859812e+38f
#endif

#if defined(__FLT_MIN__)
    #define HE_FLT_MIN __FLT_MIN__
#else
    #define HE_FLT_MIN 1.17549435082228750797e-38f
#endif

#if defined(__FLT_EPSILON__)
    #define HE_FLT_EPSILON __FLT_EPSILON__
#else
    #define HE_FLT_EPSILON 1.1920928955078125e-07f
#endif

#if defined(__FLT_DENORM_MIN__)
    #define HE_FLT_DENORM_MIN __FLT_DENORM_MIN__
#else
    #define HE_FLT_DENORM_MIN 1.40129846432481707092e-45f
#endif

// ------------------------------------------------------------------------------------------------
#if defined(__SIZEOF_DOUBLE__)
    #define HE_SIZEOF_DOUBLE __SIZEOF_DOUBLE__
#else
    #define HE_SIZEOF_DOUBLE 8
#endif

#if defined(__DBL_MANT_DIG__)
    #define HE_DBL_MANT_DIG __DBL_MANT_DIG__
#else
    #define HE_DBL_MANT_DIG 53
#endif

#if defined(__DBL_DIG__)
    #define HE_DBL_DIG __DBL_DIG__
#else
    #define HE_DBL_DIG 15
#endif

#if defined(__DBL_MAX_EXP__)
    #define HE_DBL_MAX_EXP __DBL_MAX_EXP__
#else
    #define HE_DBL_MAX_EXP 1024
#endif

#if defined(__DBL_MIN_EXP__)
    #define HE_DBL_MIN_EXP __DBL_MIN_EXP__
#else
    #define HE_DBL_MIN_EXP (-1021)
#endif

#if defined(__DBL_MAX__)
    #define HE_DBL_MAX __DBL_MAX__
#else
    #define HE_DBL_MAX 1.79769313486231570815e+308
#endif

#if defined(__DBL_MIN__)
    #define HE_DBL_MIN __DBL_MIN__
#else
    #define HE_DBL_MIN 2.22507385850720138309e-308
#endif

#if defined(__DBL_EPSILON__)
    #define HE_DBL_EPSILON __DBL_EPSILON__
#else
    #define HE_DBL_EPSILON 2.22044604925031308085e-16
#endif

#if defined(__DBL_DENORM_MIN__)
    #define HE_DBL_DENORM_MIN __DBL_DENORM_MIN__
#else
    #define HE_DBL_DENORM_MIN 4.94065645841246544177e-324
#endif

// ------------------------------------------------------------------------------------------------
#if defined(__SIZEOF_LONG_DOUBLE__)
    #define HE_SIZEOF_LONG_DOUBLE __SIZEOF_LONG_DOUBLE__
#else
    #define HE_SIZEOF_LONG_DOUBLE 8
#endif

#if defined(__LDBL_MANT_DIG__)
    #define HE_LDBL_MANT_DIG __LDBL_MANT_DIG__
#else
    #define HE_LDBL_MANT_DIG HE_DBL_MANT_DIG
#endif

#if defined(__LDBL_DIG__)
    #define HE_LDBL_DIG __LDBL_DIG__
#else
    #define HE_LDBL_DIG HE_DBL_DIG
#endif

#if defined(__LDBL_MAX_EXP__)
    #define HE_LDBL_MAX_EXP __LDBL_MAX_EXP__
#else
    #define HE_LDBL_MAX_EXP HE_DBL_MAX_EXP
#endif

#if defined(__LDBL_MIN_EXP__)
    #define HE_LDBL_MIN_EXP __LDBL_MIN_EXP__
#else
    #define HE_LDBL_MIN_EXP HE_DBL_MIN_EXP
#endif

#if defined(__LDBL_MAX__)
    #define HE_LDBL_MAX __LDBL_MAX__
#else
    #define HE_LDBL_MAX HE_DBL_MAX
#endif

#if defined(__LDBL_MIN__)
    #define HE_LDBL_MIN __LDBL_MIN__
#else
    #define HE_LDBL_MIN HE_DBL_MIN
#endif

#if defined(__LDBL_EPSILON__)
    #define HE_LDBL_EPSILON __LDBL_EPSILON__
#else
    #define HE_LDBL_EPSILON HE_DBL_EPSILON
#endif

#if defined(__LDBL_DENORM_MIN__)
    #define HE_LDBL_DENORM_MIN __LDBL_DENORM_MIN__
#else
    #define HE_LDBL_DENORM_MIN HE_DBL_DENORM_MIN
#endif

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
        static_assert(sizeof(Type) == HE_SIZEOF_FLOAT, "Unsupported float size");

        static constexpr FloatFormat Format = FloatFormat::Binary32;

        static constexpr uint32_t Digits = HE_FLT_MANT_DIG;
        static constexpr uint32_t Digits10 = HE_FLT_DIG;

        static constexpr bool IsSigned = true;
        static constexpr bool HasImplicitBit = true;
        static constexpr uint32_t Bits = 32;
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ExponentBits = 8;
        static constexpr uint32_t SignificandBits = Digits - 1;
        static_assert(SignBits + ExponentBits + SignificandBits == Bits);

        static constexpr uint32_t MaxExponent = HE_FLT_MAX_EXP;
        static constexpr int32_t MinExponent = HE_FLT_MIN_EXP;
        static constexpr float Max = HE_FLT_MAX;
        static constexpr float Min = -Max;

        static constexpr float MinPos = HE_FLT_MIN;
        static constexpr float Epsilon = HE_FLT_EPSILON;
        static constexpr float DenormMin = HE_FLT_DENORM_MIN;

        static constexpr float Infinity = __builtin_huge_valf();
        static constexpr float NaN = __builtin_nanf("0");
        static constexpr float SignalingNaN = __builtin_nansf("1");
        static constexpr float ZeroSafe = MinPos * 1000.0f;
    };

    template <>
    struct Limits<double>
    {
        using Type = double;
        static_assert(sizeof(Type) == HE_SIZEOF_DOUBLE, "Unsupported double size");

        static constexpr FloatFormat Format = FloatFormat::Binary64;

        static constexpr uint32_t Digits = HE_DBL_MANT_DIG;
        static constexpr uint32_t Digits10 = HE_DBL_DIG;

        static constexpr bool IsSigned = true;
        static constexpr bool HasImplicitBit = true;
        static constexpr uint32_t Bits = 64;
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ExponentBits = 11;
        static constexpr uint32_t SignificandBits = Digits - 1;
        static_assert(SignBits + ExponentBits + SignificandBits == Bits);

        static constexpr uint32_t MaxExponent = HE_DBL_MAX_EXP;
        static constexpr int32_t MinExponent = HE_DBL_MIN_EXP;
        static constexpr double Max = HE_DBL_MAX;
        static constexpr double Min = -Max;

        static constexpr double MinPos = HE_DBL_MIN;
        static constexpr double Epsilon = HE_DBL_EPSILON;
        static constexpr double DenormMin = HE_DBL_DENORM_MIN;

        static constexpr double Infinity = __builtin_huge_val();
        static constexpr double NaN = __builtin_nan("0");
        static constexpr double SignalingNaN = __builtin_nans("1");
        static constexpr double ZeroSafe = MinPos * 1000.0;
    };

    template <>
    struct Limits<long double>
    {
        using Type = long double;
        static_assert(sizeof(Type) == HE_SIZEOF_LONG_DOUBLE, "Unsupported long double size");

        static constexpr uint32_t Digits = HE_LDBL_MANT_DIG;
        static constexpr uint32_t Digits10 = HE_LDBL_DIG;

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

        static constexpr uint32_t MaxExponent = HE_LDBL_MAX_EXP;
        static constexpr int32_t MinExponent = HE_LDBL_MIN_EXP;
        static constexpr long double Max = HE_LDBL_MAX;
        static constexpr long double Min = -Max;

        static constexpr long double MinPos = HE_LDBL_MIN;
        static constexpr long double Epsilon = HE_LDBL_EPSILON;
        static constexpr double DenormMin = HE_LDBL_DENORM_MIN;

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

        static constexpr long double ZeroSafe = MinPos * 1000.0;
    };
}
