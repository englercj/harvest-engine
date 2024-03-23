// Copyright Chad Engler

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/limits.h"
#include "he/core/simd.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#if HE_COMPILER_MSVC
    extern "C"
    {
        float __floorf(float);
        double __floor(double);
        #pragma intrinsic(__floorf, __floor)

        float __ceilf(float);
        double __ceil(double);
        #pragma intrinsic(__ceilf, __ceil)

        float __roundf(float);
        double __round(double);
        #pragma intrinsic(__roundf, __round)
    }
#endif

namespace he
{
    // --------------------------------------------------------------------------------------------
#if HE_HAS_INT128
    template <typename T>
    using _CarrierUint = Conditional<sizeof(T) <= 4, uint32_t, Conditional<sizeof(T) <= 8, uint64_t, uint128_t>>;
#else
    template <typename T>
    using _CarrierUint = Conditional<sizeof(T) <= 4, uint32_t, uint64_t>;
#endif

    // --------------------------------------------------------------------------------------------
#if HE_LDBL_MANT_DIG == 53 && HE_LDBL_MAX_EXP == 1024
    // long double == double
#elif HE_LDBL_MANT_DIG == 64 && HE_LDBL_MAX_EXP == 16384 && HE_CPU_LITTLE_ENDIAN
    union _LdShape
    {
        long double f;
        struct
        {
            uint64_t m;
            uint16_t se;
        } i;
    };
#elif HE_LDBL_MANT_DIG == 64 && HE_LDBL_MAX_EXP == 16384 && HE_CPU_BIG_ENDIAN
    // This is the m68k variant of 80-bit long double, and this definition only works
    // on archs where the alignment requirement of uint64_t is <= 4.
    static_assert(alignof(uint64_t) <= 4, "Unsupported long double representation")
    union _LdShape
    {
        long double f;
        struct
        {
            uint16_t se;
            uint16_t pad;
            uint64_t m;
        } i;
    };
#elif HE_LDBL_MANT_DIG == 113 && HE_LDBL_MAX_EXP == 16384 && HE_CPU_LITTLE_ENDIAN
    union _LdShape
    {
        long double f;
        struct
        {
            uint64_t lo;
            uint32_t mid;
            uint16_t top;
            uint16_t se;
        } i;
        struct
        {
            uint64_t lo;
            uint64_t hi;
        } i2;
    };
#elif HE_LDBL_MANT_DIG == 113 && HE_LDBL_MAX_EXP == 16384 && HE_CPU_BIG_ENDIAN
    union _LdShape
    {
        long double f;
        struct
        {
            uint16_t se;
            uint16_t top;
            uint32_t mid;
            uint64_t lo;
        } i;
        struct
        {
            uint64_t hi;
            uint64_t lo;
        } i2;
    };
#else
    #error "Unsupported long double representation"
#endif

    // --------------------------------------------------------------------------------------------
#if HE_FLOAT_EVAL_METHOD == 0 || HE_FLOAT_EVAL_METHOD == -1
    static constexpr float_t _FltToInt = 1 / HE_FLT_EPSILON;
    static constexpr double_t _DblToInt = 1 / HE_DBL_EPSILON;
    static constexpr long double _LdblToInt = 1 / HE_LDBL_EPSILON;
#elif HE_FLOAT_EVAL_METHOD == 1
    static constexpr float_t _FltToInt = 1 / HE_DBL_EPSILON;
    static constexpr double_t _DblToInt = 1 / HE_DBL_EPSILON;
    static constexpr long double _LdblToInt = 1 / HE_LDBL_EPSILON;
#elif HE_FLOAT_EVAL_METHOD == 2
    static constexpr float_t _FltToInt = 1 / HE_LDBL_EPSILON;
    static constexpr double_t _DblToInt = 1 / HE_LDBL_EPSILON;
    static constexpr long double _LdblToInt = 1 / HE_LDBL_EPSILON;
#endif

    // --------------------------------------------------------------------------------------------
    template <typename T>
    HE_FORCE_INLINE constexpr FpClass _ClassifyImpl(T x) noexcept
    {
        using Uint = _CarrierUint<T>;

        constexpr Uint Mask = Limits<T>::SignificandBits == 23 ? 0xffu : 0x7ffu;

        const Uint bits = BitCast<Uint>(x);
        const Uint e = (bits >> Limits<T>::SignificandBits) & Mask;

        if (e == 0)
            return (bits << Limits<T>::SignBits) ? FpClass::Subnormal : FpClass::Zero;

        if (e == Mask)
            return (bits << (Limits<T>::SignBits + Limits<T>::ExponentBits)) ? FpClass::Nan : FpClass::Infinite;

        return FpClass::Normal;
    }

    constexpr FpClass Classify(float x) noexcept { return _ClassifyImpl<float>(x); }
    constexpr FpClass Classify(double x) noexcept { return _ClassifyImpl<double>(x); }

    constexpr FpClass Classify(long double x) noexcept
    {
    #if HE_LDBL_MANT_DIG == HE_DBL_MANT_DIG && HE_LDBL_MAX_EXP == HE_DBL_MAX_EXP
        return Classify(static_cast<double>(x));
    #elif HE_LDBL_MANT_DIG == 64 && HE_LDBL_MAX_EXP == 16384
        const _LdShape u{ x };
        const uint32_t e = u.i.se & 0x7fff;
        const uint64_t msb = u.i.m >> 63;

        if (e == 0 && msb == 0)
            return u.i.m ? FpClass::Subnormal : FpClass::Zero;

        if (e == 0x7fff)
            return msb == 1 ? FpClass::Infinite : FpClass::Nan;

        return FpClass::Normal;
    #elif HE_LDBL_MANT_DIG == 113 && HE_LDBL_MAX_EXP == 16384
        _LdShape u{ x };
        const uint32_t e = u.i.se & 0x7fff;
        u.i.se = 0;

        if (e == 0)
            return u.i2.lo | u.i2.hi ? FpClass::Subnormal : FpClass::Zero;

        if (e == 0x7fff)
            return u.i2.lo | u.i2.hi ? FpClass::Nan : FpClass::Infinite;

        return FpClass::Normal;
    #else
        #error "Unsupported long double representation"
    #endif
    }

    // --------------------------------------------------------------------------------------------
    constexpr bool IsNan(float x) noexcept { return (BitCast<uint32_t>(x) & 0x7fffffffu) > 0x7f800000u; }
    constexpr bool IsNan(double x) noexcept { return (BitCast<uint64_t>(x) & (static_cast<uint64_t>(-1) >> 1)) > (0x7ffull << 52); }
    constexpr bool IsNan(long double x) noexcept { return Classify(x) == FpClass::Nan; }

    // --------------------------------------------------------------------------------------------
    constexpr bool IsInfinite(float x) noexcept { return (BitCast<uint32_t>(x) & 0x7fffffffu) == 0x7f800000u; }
    constexpr bool IsInfinite(double x) noexcept { return (BitCast<uint64_t>(x) & (static_cast<uint64_t>(-1) >> 1)) == (0x7ffull << 52); }
    constexpr bool IsInfinite(long double x) noexcept { return Classify(x) == FpClass::Infinite; }

    // --------------------------------------------------------------------------------------------
    constexpr bool IsFinite(float x) noexcept { return (BitCast<uint32_t>(x) & 0x7fffffffu) < 0x7f800000u; }
    constexpr bool IsFinite(double x) noexcept { return (BitCast<uint64_t>(x) & (static_cast<uint64_t>(-1) >> 1)) < (0x7ffull << 52); }
    constexpr bool IsFinite(long double x) noexcept { const FpClass cls = Classify(x); return cls != FpClass::Infinite && cls != FpClass::Nan; }

    // --------------------------------------------------------------------------------------------
    constexpr bool IsNormal(float x) noexcept { return ((BitCast<uint32_t>(x) + 0x00800000u) & 0x7fffffffu) >= 0x01000000u; }
    constexpr bool IsNormal(double x) noexcept { return ((BitCast<uint64_t>(x) + (1ull << 52)) & (static_cast<uint64_t>(-1) >> 1)) >= (1ull << 53); }
    constexpr bool IsNormal(long double x) noexcept { return Classify(x)== FpClass::Normal; }

    // --------------------------------------------------------------------------------------------
    constexpr bool IsZeroSafe(float x) noexcept { return Abs(x) > Limits<float>::ZeroSafe; }
    constexpr bool IsZeroSafe(double x) noexcept { return Abs(x) > Limits<double>::ZeroSafe; }
    constexpr bool IsZeroSafe(long double x) noexcept { return Abs(x) > Limits<long double>::ZeroSafe; }

    // --------------------------------------------------------------------------------------------
    constexpr bool IsNearlyEqual(float a, float b, float tolerance) noexcept { return Abs(a - b) <= tolerance; }
    constexpr bool IsNearlyEqual(double a, double b, double tolerance) noexcept { return Abs(a - b) <= tolerance; }
    constexpr bool IsNearlyEqual(long double a, long double b, long double tolerance) noexcept { return Abs(a - b) <= tolerance; }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    HE_FORCE_INLINE constexpr bool _IsNearlyEqualULPImpl(T a, T b, uint32_t maxUlpDiff) noexcept
    {
        // Any comparison of NaN is always false.
        if (IsNan(a) || IsNan(b))
            return false;

        // If either is infinite, then do a direct comparison.
        // This ensures `inifinity == -infinity` but `FLT_MAX != infinity` even though their bit
        // representations are very close.
        if (IsInfinite(a) || IsInfinite(b))
            return a == b;

        // By converting the float to an integer representation, we can compare the ULP
        // difference between the two, even when the values are close to zero.
        using Uint = _CarrierUint<T>;
        constexpr Uint SignBit = Uint(1) << (Limits<T>::Bits - 1);

        const Uint au = BitCast<Uint>(a);
        const Uint bu = BitCast<Uint>(b);
        const Uint ai = (au & SignBit) ? (~au + 1) : (au | SignBit);
        const Uint bi = (bu & SignBit) ? (~bu + 1) : (bu | SignBit);

        // Calculate the difference and compare it to the maximum ULP difference allowed.
        const Uint ulpDiff = (ai > bi) ? (ai - bi) : (bi - ai);
        return ulpDiff <= static_cast<Uint>(maxUlpDiff);
    }

    constexpr bool IsNearlyEqualULP(float a, float b, uint32_t maxUlpDiff) noexcept { return _IsNearlyEqualULPImpl<float>(a, b, maxUlpDiff); }
    constexpr bool IsNearlyEqualULP(double a, double b, uint32_t maxUlpDiff) noexcept { return _IsNearlyEqualULPImpl<double>(a, b, maxUlpDiff); }
    constexpr bool IsNearlyEqualULP(long double a, long double b, uint32_t maxUlpDiff) noexcept { return _IsNearlyEqualULPImpl<long double>(a, b, maxUlpDiff); }

    // --------------------------------------------------------------------------------------------
    constexpr bool HasSignBit(float x) noexcept { return BitCast<uint32_t>(x) >> (Limits<float>::Bits - 1); }
    constexpr bool HasSignBit(double x) noexcept { return BitCast<uint64_t>(x) >> (Limits<double>::Bits - 1); }

    constexpr bool HasSignBit(long double x) noexcept
    {
    #if HE_LDBL_MANT_DIG == HE_DBL_MANT_DIG && HE_LDBL_MAX_EXP == HE_DBL_MAX_EXP
        return HasSignBit(static_cast<double>(x));
    #elif (HE_LDBL_MANT_DIG == 64 || HE_LDBL_MANT_DIG == 113) && HE_LDBL_MAX_EXP == 16384
        const _LdShape u{ x };
        return static_cast<bool>(u.i.se >> 15);
    #else
        #error "Unsupported long double representation"
    #endif
    }

    // --------------------------------------------------------------------------------------------
    constexpr float Floor(float x) noexcept
    {
        if (!IsConstantEvaluated())
        {
        #if HE_HAS_BUILTIN(__builtin_floorf)
            // `f32.floor` instruction on WASM. Compiler impl or library call on other platforms.
            return __builtin_floorf(x);
        #elif HE_COMPILER_MSVC
            return __floorf(x);
        #endif
        }

        const uint32_t bits = BitCast<uint32_t>(x);
        const int32_t e = static_cast<int32_t>((bits >> 23) & 0xff) - 0x7f;

        if (e >= 23)
            return x;

        if (e >= 0)
        {
            const uint32_t m = 0x007fffffu >> e;
            if ((bits & m) == 0)
                return x;

            // FORCE_EVAL(x + 0x1p120f); // we don't care about float exceptions
            uint32_t ret = bits;
            if (ret >> 31)
                ret += m;

            ret &= ~m;
            return BitCast<float>(ret);
        }

        // FORCE_EVAL(x + 0x1p120f); // we don't care about float exceptions
        if ((bits >> 31) == 0)
            return 0.0f;

        if (bits << 1)
            return -1.0f;

        return x;
    }

    constexpr double Floor(double x) noexcept
    {
        if (!IsConstantEvaluated())
        {
        #if HE_HAS_BUILTIN(__builtin_floor)
            // `f64.floor` instruction on WASM. Compiler impl or library call on other platforms.
            return __builtin_floor(x);
        #elif HE_COMPILER_MSVC
            return __floor(x);
        #endif
        }

        const uint64_t bits = BitCast<uint64_t>(x);
        const int64_t e = static_cast<int64_t>((bits >> Limits<double>::SignificandBits) & 0x7ff);

        if (e >= (0x3ff + 52) || x == 0)
            return x;

        // y = int(x) - x, where int(x) is an integer neighbor of x
        double_t y = 0.0;
        if (bits >> 63)
            y = x - _DblToInt + _DblToInt - x;
        else
            y = x + _DblToInt - _DblToInt - x;

        // special case because of non-nearest rounding modes
        if (e <= (0x3ff - 1))
        {
            // FORCE_EVAL(y); // we don't care about float exceptions
            return (bits >> 63) ? -1.0f : 0.0f;
        }

        return (y > 0) ? (x + y - 1) : (x + y);
    }

    constexpr long double Floor(long double x) noexcept
    {
    #if HE_LDBL_MANT_DIG == HE_DBL_MANT_DIG && HE_LDBL_MAX_EXP == HE_DBL_MAX_EXP
        return Floor(static_cast<double>(x));
    #elif (HE_LDBL_MANT_DIG == 64 || HE_LDBL_MANT_DIG == 113) && HE_LDBL_MAX_EXP == 16384
        _LdShape u{ x };
        const uint32_t e = u.i.se & 0x7fff;

        if (e >= (0x3fff + Limits<long double>::SignificandBits) || x == 0)
            return x;

        // y = int(x) - x, where int(x) is an integer neighbor of x
        long double y = 0.0;
        if (u.i.se >> 15)
            y = x - _LdblToInt + _LdblToInt - x;
        else
            y = x + _LdblToInt - _LdblToInt - x;

        // special case because of non-nearest rounding modes
        if (e <= (0x3fff - 1))
        {
            // FORCE_EVAL(y); // we don't care about float exceptions
            return u.i.se >> 15 ? -1.0 : 0.0;
        }

        return (y > 0) ? (x + y - 1) : (x + y);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    constexpr float Ceil(float x) noexcept
    {
        if (!IsConstantEvaluated())
        {
        #if HE_HAS_BUILTIN(__builtin_ceilf)
            // `f32.ceil` instruction on WASM. Compiler impl or library call on other platforms.
            return __builtin_ceilf(x);
        #elif HE_COMPILER_MSVC
            return __ceilf(x);
        #endif
        }

        const uint32_t bits = BitCast<uint32_t>(x);
        const int32_t e = static_cast<int32_t>((bits >> 23) & 0xff) - 0x7f;

        if (e >= 23)
            return x;

        if (e >= 0)
        {
            const uint32_t m = 0x007fffffu >> e;
            if ((bits & m) == 0)
                return x;

            // FORCE_EVAL(x + 0x1p120f); // we don't care about float exceptions
            uint32_t ret = bits;
            if ((ret >> 31) == 0)
                ret += m;

            ret &= ~m;
            return BitCast<float>(ret);
        }

        // FORCE_EVAL(x + 0x1p120f); // we don't care about float exceptions
        if (bits >> 31)
            return -0.0f;

        if (bits << 1)
            return 1.0f;

        return x;
    }

    constexpr double Ceil(double x) noexcept
    {
        if (!IsConstantEvaluated())
        {
        #if HE_HAS_BUILTIN(__builtin_ceil)
            // `f64.ceil` instruction on WASM. Compiler impl or library call on other platforms.
            return __builtin_ceil(x);
        #elif HE_COMPILER_MSVC
            return __ceil(x);
        #endif
        }

        const uint64_t bits = BitCast<uint64_t>(x);
        const int64_t e = static_cast<int64_t>((bits >> Limits<double>::SignificandBits) & 0x7ff);

        if (e >= (0x3ff + 52) || x == 0)
            return x;

        // y = int(x) - x, where int(x) is an integer neighbor of x
        double_t y = 0.0;
        if (bits >> 63)
            y = x - _DblToInt + _DblToInt - x;
        else
            y = x + _DblToInt - _DblToInt - x;

        // special case because of non-nearest rounding modes
        if (e <= (0x3ff - 1))
        {
            // FORCE_EVAL(y); // we don't care about float exceptions
            return (bits >> 63) ? -0.0 : 1.0;
        }

        return (y < 0) ? (x + y + 1) : (x + y);
    }

    constexpr long double Ceil(long double x) noexcept
    {
    #if HE_LDBL_MANT_DIG == HE_DBL_MANT_DIG && HE_LDBL_MAX_EXP == HE_DBL_MAX_EXP
        return Ceil(static_cast<double>(x));
    #elif (HE_LDBL_MANT_DIG == 64 || HE_LDBL_MANT_DIG == 113) && HE_LDBL_MAX_EXP == 16384
        _LdShape u{ x };
        const uint32_t e = u.i.se & 0x7fff;

        if (e >= (0x3fff + Limits<long double>::SignificandBits) || x == 0)
            return x;

        // y = int(x) - x, where int(x) is an integer neighbor of x
        long double y = 0.0;
        if (u.i.se >> 15)
            y = x - _LdblToInt + _LdblToInt - x;
        else
            y = x + _LdblToInt - _LdblToInt - x;

        // special case because of non-nearest rounding modes
        if (e <= (0x3fff - 1))
        {
            // FORCE_EVAL(y); // we don't care about float exceptions
            return u.i.se >> 15 ? -0.0 : 1.0;
        }

        return (y > 0) ? (x + y + 1) : (x + y);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    constexpr float Round(float x) noexcept
    {
        if (!IsConstantEvaluated())
        {
        #if !defined(HE_PLATFORM_API_WASM) && HE_HAS_BUILTIN(__builtin_roundf)
            // Compiler impl or library call, no WASM instruction here.
            return __builtin_roundf(x);
        #elif HE_COMPILER_MSVC
            return __roundf(x);
        #endif
        }

    #if HE_FLOAT_EVAL_METHOD == -1
        // Generally less accurate on some edge cases, but when the float evaluation mode is
        // unknown we can't rely on rounding behavior. Indeed when using `/fp:Fast` on MSVC
        // this gives us the correct answer more often.
        return static_cast<float>(static_cast<int32_t>(x >= 0.0f ? (x + 0.5f) : (x - 0.5f)));
    #else
        const uint32_t bits = BitCast<uint32_t>(x);
        const uint32_t e = (bits >> 23) & 0xff;

        if (e >= (0x7f + 23))
            return x;

        const float abs = (bits >> 31) ? -x : x;

        if (e < (0x7f - 1))
        {
            // FORCE_EVAL(abs + _FltToInt); // we don't care about float exceptions
            return 0.0f * x;
        }

        float_t y = abs + _FltToInt - _FltToInt - abs;
        if (y > 0.5f)
            y = y + abs - 1;
        else if (y <= -0.5f)
            y = y + abs + 1;
        else
            y = y + abs;

        if (bits >> 31)
            y = -y;

        return y;
    #endif
    }

    constexpr double Round(double x) noexcept
    {
        if (!IsConstantEvaluated())
        {
        #if !defined(HE_PLATFORM_API_WASM) && HE_HAS_BUILTIN(__builtin_round)
            // Compiler impl or library call, no WASM instruction here.
            return __builtin_round(x);
        #elif HE_COMPILER_MSVC
            return __round(x);
        #endif
        }

    #if HE_FLOAT_EVAL_METHOD == -1
        // Generally less accurate on some edge cases, but when the float evaluation mode is
        // unknown we can't rely on rounding behavior. Indeed when using `/fp:Fast` on MSVC
        // this gives us the correct answer more often.
        return static_cast<double>(static_cast<int64_t>(x >= 0.0 ? (x + 0.5) : (x - 0.5)));
    #else
        const uint64_t bits = BitCast<uint64_t>(x);
        const uint64_t e = (bits >> 52) & 0x7ff;

        if (e >= (0x3ff + 52))
            return x;

        const double abs = (bits >> 63) ? -x : x;

        if (e < (0x3ff - 1))
        {
            // FORCE_EVAL(abs + _DblToInt); // we don't care about float exceptions
            return 0.0 * x;
        }

        double_t y = abs + _DblToInt - _DblToInt - abs;
        if (y > 0.5)
            y = y + abs - 1;
        else if (y <= -0.5)
            y = y + abs + 1;
        else
            y = y + abs;

        if (bits >> 63)
            y = -y;

        return y;
    #endif
    }

    constexpr long double Round(long double x) noexcept
    {
    #if HE_LDBL_MANT_DIG == HE_DBL_MANT_DIG && HE_LDBL_MAX_EXP == HE_DBL_MAX_EXP
        return Round(static_cast<double>(x));
    #elif (HE_LDBL_MANT_DIG == 64 || HE_LDBL_MANT_DIG == 113) && HE_LDBL_MAX_EXP == 16384
        _LdShape u{ x };
        const uint32_t e = u.i.se & 0x7fff;

        if (e >= (0x3fff + Limits<long double>::SignificandBits))
            return x;

        const double abs = (u.i.se >> 15) ? -x : x;

        if (e < (0x3fff - 1))
        {
            // FORCE_EVAL(abs + _LdblToInt); // we don't care about float exceptions
            return 0 * x;
        }

        long double y = abs + _LdblToInt - _LdblToInt - abs;
        if (y > 0.5)
            y = y + abs - 1;
        else if (y <= -0.5)
            y = y + abs + 1;
        else
            y = y + abs;

        if (u.i.se >> 15)
            y = -y;

        return y;
    #endif
    }

    // --------------------------------------------------------------------------------------------
    constexpr float ToRadians(float deg) noexcept { return (deg * MathConstants<float>::Pi) / 180.0f; }
    constexpr double ToRadians(double deg) noexcept { return (deg * MathConstants<double>::Pi) / 180.0; }
    constexpr long double ToRadians(long double deg) noexcept { return (deg * MathConstants<long double>::Pi) / 180.0l; }

    // --------------------------------------------------------------------------------------------
    constexpr float ToDegrees(float rad) noexcept { return (rad * 180.0f) / MathConstants<float>::Pi; }
    constexpr double ToDegrees(double rad) noexcept { return (rad * 180.0) / MathConstants<double>::Pi; }
    constexpr long double ToDegrees(long double rad) noexcept { return (rad * 180.0l) / MathConstants<long double>::Pi; }

    // --------------------------------------------------------------------------------------------
    constexpr float Lerp(float a, float b, float t) noexcept { return a + ((b - a) * t); }
    constexpr double Lerp(double a, double b, double t) noexcept { return a + ((b - a) * t); }
    constexpr long double Lerp(long double a, long double b, long double t) noexcept { return a + ((b - a) * t); }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    HE_FORCE_INLINE constexpr T _SmoothStepImpl(T a, T b, T t) noexcept
    {
        const T c = Clamp<T>((t - a) / (b - a), T(0), T(1));
        return c * c * (T(3) - (T(2) * c));
    }

    constexpr float SmoothStep(float a, float b, float t) noexcept { return _SmoothStepImpl<float>(a, b, t); }
    constexpr double SmoothStep(double a, double b, double t) noexcept { return _SmoothStepImpl<double>(a, b, t); }
    constexpr long double SmoothStep(long double a, long double b, long double t) noexcept { return _SmoothStepImpl<long double>(a, b, t); }

    // --------------------------------------------------------------------------------------------
    constexpr float Rcp(float x) noexcept { return 1.0f / x; }
    constexpr double Rcp(double x) noexcept { return 1.0 / x; }
    constexpr long double Rcp(long double x) noexcept { return 1.0l / x; }

    // --------------------------------------------------------------------------------------------
    constexpr float RcpSafe(float x) noexcept { return IsZeroSafe(x) ? (1.0f / x) : 0.0f; }
    constexpr double RcpSafe(double x) noexcept { return IsZeroSafe(x) ? (1.0 / x) : 0.0; }
    constexpr long double RcpSafe(long double x) noexcept { return IsZeroSafe(x) ? (1.0l / x) : 0.0l; }

    // --------------------------------------------------------------------------------------------
    HE_FORCE_INLINE float Sqrt(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        // `f32.sqrt` instruction on WASM.
        return __builtin_sqrtf(x);
    #elif HE_SIMD_SSE2
        const __m128 v = _mm_set_ss(x);
        const __m128 r = _mm_sqrt_ss(v);
        return _mm_cvtss_f32(r);
    #elif HE_SIMD_NEON && HE_CPU_ARM_64
        const float32x4_t v = vdupq_n_f32(x);
        const float32x4_t r = vsqrtq_f32(v);
        return vgetq_lane_f32(r, 0);
    #else
        #error "No Sqrt implementation"
    #endif
    }

    HE_FORCE_INLINE double Sqrt(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        // `f64.sqrt` instruction on WASM.
        return __builtin_sqrt(x);
    #elif HE_SIMD_SSE2
        const __m128d v = _mm_set_sd(x);
        const __m128d r = _mm_sqrt_sd(_mm_setzero_pd(), v);
        return _mm_cvtsd_f64(r);
    #elif HE_SIMD_NEON && HE_CPU_ARM_64
        const float64x2_t v = vdupq_n_f64(x);
        const float64x2_t r = vsqrtq_f64(v);
        return vgetq_lane_f64(r, 0);
    #else
        #error "No Sqrt implementation"
    #endif
    }

    // --------------------------------------------------------------------------------------------
    HE_FORCE_INLINE float Rsqrt(float x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        // `f32.div`, `f64.sqrt` instructions on WASM.
        return 1.0f / __builtin_sqrtf(x);
    #elif HE_SIMD_SSE2
        const __m128 one = _mm_set_ss(1.0f);
        const __m128 v = _mm_set_ss(x);
        const __m128 s = _mm_sqrt_ss(v);
        const __m128 r = _mm_div_ss(one, s);
        return _mm_cvtss_f32(r);
    #elif HE_SIMD_NEON && HE_CPU_ARM_64
        const float32x4_t one = vdupq_n_f32(1.0f);
        const float32x4_t v = vdupq_n_f32(x);
        const float32x4_t s = vsqrtq_f32(v);
        const float32x4_t r = vdivq_f32(one, s);
        return vgetq_lane_f32(r, 0);
    #else
        return 1.0f / Sqrt(x);
    #endif
    }

    HE_FORCE_INLINE double Rsqrt(double x) noexcept
    {
    #if defined(HE_PLATFORM_API_WASM)
        // `f64.div`, `f64.sqrt` instructions on WASM.
        return 1.0 / __builtin_sqrt(x);
    #elif HE_SIMD_SSE2
        const __m128d one = _mm_set_sd(1.0);
        const __m128d v = _mm_set_sd(x);
        const __m128d s = _mm_sqrt_sd(_mm_setzero_pd(), v);
        const __m128d r = _mm_div_sd(one, s);
        return _mm_cvtsd_f64(r);
    #elif HE_SIMD_NEON && HE_CPU_ARM_64
        const float64x2_t one = vdupq_n_f64(1.0);
        const float64x2_t v = vdupq_n_f64(x);
        const float64x2_t s = vsqrtq_f64(v);
        const float64x2_t r = vdivq_f64(one, s);
        return vgetq_lane_f64(r, 0);
    #else
        return 1.0 / Sqrt(x);
    #endif
    }

    // --------------------------------------------------------------------------------------------
    HE_FORCE_INLINE float LogN(float x, float n) noexcept { return Ln(x) / Ln(n); }
    HE_FORCE_INLINE double LogN(double x, double n) noexcept { return Ln(x) / Ln(n); }
}
