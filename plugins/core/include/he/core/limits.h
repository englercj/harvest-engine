// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/types.h"

namespace he
{
    template <typename T>
    struct Limits;

    template <SignedIntegral T>
    struct Limits<T>
    {
        using Type = T;

        static constexpr bool IsSigned = true;
        static constexpr uint32_t Bits = (sizeof(T) * 8);
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ValueBits = static_cast<uint32_t>(Bits - SignBits);

        HE_PUSH_WARNINGS();
        HE_DISABLE_MSVC_WARNING(4310);
        static constexpr T Min = T(T(1) << ValueBits);
        static constexpr T Max = T(T(~0) ^ Min);
        HE_POP_WARNINGS();
    };

    template <UnsignedIntegral T>
    struct Limits<T>
    {
        using Type = T;

        static constexpr bool IsSigned = false;
        static constexpr uint32_t Bits = (sizeof(T) * 8);
        static constexpr uint32_t SignBits = 0;
        static constexpr uint32_t ValueBits = Bits;

        static constexpr T Min = T(0);
        static constexpr T Max = T(~0);
    };

    template <>
    struct Limits<bool>
    {
        using Type = bool;

        static constexpr bool IsSigned = false;
        static constexpr uint32_t Bits = 8;
        static constexpr uint32_t SignBits = 0;
        static constexpr uint32_t ValueBits = 8;

        static constexpr bool Min = false;
        static constexpr bool Max = true;
    };

    template <>
    struct Limits<float>
    {
        using Type = float;

        static constexpr bool IsSigned = true;
        static constexpr uint32_t Bits = 32;
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ExponentBits = 8;
        static constexpr uint32_t MantissaBits = 24;

        static constexpr float Min = -3.402823466e+38f;
        static constexpr float Max = 3.402823466e+38f;

        static constexpr float MinPos = 1.175494351e-38f;
        static constexpr float Epsilon = 1.192092896e-07f;
        static constexpr float Infinity = __builtin_huge_valf();
        static constexpr float NaN = __builtin_nanf("");
        static constexpr float SignalingNaN = __builtin_nansf("");
    };

    template <>
    struct Limits<double>
    {
        using Type = double;

        static constexpr bool IsSigned = true;
        static constexpr uint32_t Bits = 64;
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ExponentBits = 11;
        static constexpr uint32_t MantissaBits = 53;

        static constexpr double Min = -1.7976931348623158e+308;
        static constexpr double Max = 1.7976931348623158e+308;

        static constexpr double MinPos = 2.2250738585072014e-308;
        static constexpr double Epsilon = 2.2204460492503131e-016;
        static constexpr double Infinity = __builtin_huge_val();
        static constexpr double NaN = __builtin_nan("");
        static constexpr double SignalingNaN = __builtin_nans("");
    };

    template <>
    struct Limits<long double>
    {
        using Type = long double;

        static constexpr bool IsSigned = true;
        static constexpr uint32_t Bits = 64;
        static constexpr uint32_t SignBits = 1;
        static constexpr uint32_t ExponentBits = 11;
        static constexpr uint32_t MantissaBits = 53;

        static constexpr double Min = -1.7976931348623158e+308;
        static constexpr double Max = 1.7976931348623158e+308;

        static constexpr double MinPos = 2.2250738585072014e-308;
        static constexpr double Epsilon = 2.2204460492503131e-016;
        static constexpr double Infinity = __builtin_huge_val();
        static constexpr double NaN = __builtin_nan("");
        static constexpr double SignalingNaN = __builtin_nans("");
    };
}
