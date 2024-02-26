// Copyright Chad Engler

// This file is based on the implementation in {fmt}
// https://fmt.dev/
// https://github.com/fmtlib/fmt
//
// License:
//
// Copyright (c) 2012 - present, Victor Zverovich
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// --- Optional exception to the license ---
//
// As an exception, if, as a result of your compiling your source code, portions of this Software
// are embedded into a machine-executable object form of such source code, you may redistribute
// such embedded portions in such object form without including the above copyright and permission
// notices.

#pragma once

#include "fmt_private.h"

#include "he/core/compiler.h"
#include "he/core/limits.h"
#include "he/core/memory_ops.h"
#include "he/core/types.h"

#if HE_COMPILER_MSVC
    extern "C" unsigned __int64 _umul128(unsigned __int64 Multiplier, unsigned __int64 Multiplicand, unsigned __int64* HighProduct);
    #pragma intrinsic(_umul128)

    extern "C" unsigned __int64 __umulh(unsigned __int64 a, unsigned __int64 b);
    #pragma intrinsic(__umulh)
#endif

namespace he::dragonbox
{
    // Computes floor(log10(pow(2, e))) for e in [-2620, 2620] using the method from
    // https://fmt.dev/papers/Dragonbox.pdf#page=28, section 6.1.
    inline int32_t FloorLog10Pow2(int e) noexcept
    {
        HE_FMT_ASSERT(e <= 2620 && e >= -2620, "too large exponent");
        static_assert((-1 >> 1) == -1, "right shift is not arithmetic");
        return (e * 315653) >> 20;
    }

    inline int32_t FloorLog2Pow10(int e) noexcept
    {
        HE_FMT_ASSERT(e <= 1233 && e >= -1233, "too large exponent");
        return (e * 1741647) >> 19;
    }

    // Computes 128-bit result of multiplication of two 64-bit unsigned integers.
    static uint128_t UMul128(uint64_t x, uint64_t y) noexcept
    {
    #if HE_HAS_INT128
        return static_cast<uint128_t>(x) * static_cast<uint128_t>(y);
    #elif HE_COMPILER_MSVC
        auto hi = uint64_t();
        auto lo = _umul128(x, y, &hi);
        return { hi, lo };
    #else
        const uint64_t mask = static_cast<uint64_t>(Limits<uint32_t>::Max);

        const uint64_t a = x >> 32;
        const uint64_t b = x & mask;
        const uint64_t c = y >> 32;
        const uint64_t d = y & mask;

        const uint64_t ac = a * c;
        const uint64_t bc = b * c;
        const uint64_t ad = a * d;
        const uint64_t bd = b * d;

        const uint64_t intermediate = (bd >> 32) + (ad & mask) + (bc & mask);

        const uint64_t hi = ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32);
        const uint64_t lo = (intermediate << 32) + (bd & mask);

        return { hi, lo };
    #endif
    }

    // Computes upper 64 bits of multiplication of two 64-bit unsigned integers.
    inline uint64_t UMul128Upper64(uint64_t x, uint64_t y) noexcept
    {
    #if HE_HAS_INT128
        const uint128_t p = static_cast<uint128_t>(x) * static_cast<uint128_t>(y);
        return static_cast<uint64_t>(p >> 64);
    #elif HE_COMPILER_MSVC
        return __umulh(x, y);
    #else
        return UMul128(x, y).High();
    #endif
    }

    // Computes upper 128 bits of multiplication of a 64-bit unsigned integer and a
    // 128-bit unsigned integer.
    inline uint128_t UMul192Upper128(uint64_t x, uint128_t y) noexcept
    {
        uint128_t r = UMul128(x, HE_UINT128_HIGH64(y));
        r += UMul128Upper64(x, HE_UINT128_LOW64(y));
        return r;
    }

    // Computes lower 128 bits of multiplication of a 64-bit unsigned integer and a
    // 128-bit unsigned integer.
    inline uint128_t UMul192Lower128(uint64_t x, uint128_t y) noexcept {
        const uint64_t high = x * HE_UINT128_HIGH64(y);
        const uint128_t high_low = UMul128(x, HE_UINT128_LOW64(y));
    #if HE_HAS_INT128
        return (uint128_t(high) << 64) + high_low;
    #else
        return { high + HE_UINT128_HIGH64(high_low), HE_UINT128_LOW64(high_low) };
    #endif
    }

    // Computes upper 64 bits of multiplication of a 32-bit unsigned integer and a
    // 64-bit unsigned integer.
    inline uint64_t UMul96Upper64(uint32_t x, uint64_t y) noexcept
    {
        return UMul128Upper64(static_cast<uint64_t>(x) << 32, y);
    }

    // Computes lower 64 bits of multiplication of a 32-bit unsigned integer and a
    // 64-bit unsigned integer.
    inline uint64_t UMul96Lower64(uint32_t x, uint64_t y) noexcept
    {
        return x * y;
    }

    // Various fast log computations.
    inline int32_t FloorLog10Pow2MinusLog10Of4Over3(int32_t e) noexcept
    {
        HE_FMT_ASSERT(e <= 2936 && e >= -2985, "too large exponent");
        return (e * 631305 - 261663) >> 21;
    }

    inline constexpr struct
    {
        uint32_t divisor;
        int shiftAmount;
    } DivSmallPow10Infos[] = { { 10, 16 }, { 100, 16 } };

    // Replaces n by floor(n / pow(10, N)) returning true if and only if n is
    // divisible by pow(10, N).
    // Precondition: n <= pow(10, N + 1).
    template <int32_t N>
    bool CheckDivisibilityAndDivideByPow10(uint32_t& n) noexcept
    {
        // The numbers below are chosen such that:
        //   1. floor(n/d) = floor(nm / 2^k) where d=10 or d=100,
        //   2. nm mod 2^k < m if and only if n is divisible by d,
        // where m is magicNumber, k is shiftAmount
        // and d is divisor.
        //
        // Item 1 is a common technique of replacing division by a constant with
        // multiplication, see e.g. "Division by Invariant Integers Using
        // Multiplication" by Granlund and Montgomery (1994). magicNumber (m) is set
        // to ceil(2^k/d) for large enough k.
        // The idea for item 2 originates from Schubfach.
        constexpr auto DivInfo = DivSmallPow10Infos[N - 1];
        HE_FMT_ASSERT(n <= DivInfo.divisor * 10, "n is too large");
        constexpr uint32_t MagicNumber = (1u << DivInfo.shiftAmount) / DivInfo.divisor + 1;
        n *= MagicNumber;
        const uint32_t comparisonMask = (1u << DivInfo.shiftAmount) - 1;
        const bool result = (n & comparisonMask) < MagicNumber;
        n >>= DivInfo.shiftAmount;
        return result;
    }

    // Computes floor(n / pow(10, N)) for small n and N.
    // Precondition: n <= pow(10, N + 1).
    template <int32_t N>
    uint32_t SmallDivisionByPow10(uint32_t n) noexcept
    {
        constexpr auto DivInfo = DivSmallPow10Infos[N - 1];
        HE_FMT_ASSERT(n <= DivInfo.divisor * 10, "n is too large");
        constexpr uint32_t magicNumber = (1u << DivInfo.shiftAmount) / DivInfo.divisor + 1;
        return (n * magicNumber) >> DivInfo.shiftAmount;
    }

    // Computes floor(n / 10^(kappa + 1)) (float)
    inline uint32_t DivideBy10ToKappaPlus1(uint32_t n) noexcept
    {
        // 1374389535 = ceil(2^37/100)
        return static_cast<uint32_t>((static_cast<uint64_t>(n) * 1374389535) >> 37);
    }

    // Computes floor(n / 10^(kappa + 1)) (double)
    inline uint64_t DivideBy10ToKappaPlus1(uint64_t n) noexcept
    {
        // 2361183241434822607 = ceil(2^(64+7)/1000)
        return UMul128Upper64(n, 2361183241434822607ull) >> 7;
    }

    template <typename T> struct CacheAccessor;

    template <>
    struct CacheAccessor<float>
    {
        using Uint = CarrierUint<float>;
        using EntryType = uint64_t;
        static constexpr int32_t MinK = -31;
        static constexpr int32_t MaxK = 46;
        static constexpr int32_t Kappa = 1;
        static constexpr int32_t BigDivisor = 100;
        static constexpr int32_t SmallDivisor = 10;
        static constexpr int32_t ShorterIntervalTieLowerThreshold = -35;
        static constexpr int32_t ShorterIntervalTieUpperThreshold = -35;

        static EntryType GetCachedPower(int32_t k) noexcept;

        static uint32_t ComputeDelta(const EntryType& cache, int32_t beta) noexcept
        {
            return static_cast<uint32_t>(cache >> (64 - 1 - beta));
        }

        struct ComputeMulResult
        {
            Uint result;
            bool isInteger;
        };

        static ComputeMulResult ComputeMul(Uint u, const EntryType& cache) noexcept
        {
            const uint64_t r = UMul96Upper64(u, cache);
            return { static_cast<Uint>(r >> 32), static_cast<Uint>(r) == 0 };
        }

        struct ComputeMulParityResult
        {
            bool parity;
            bool isInteger;
        };
        static ComputeMulParityResult ComputeMulParity(Uint twof, const EntryType& cache, int32_t beta) noexcept
        {
            HE_FMT_ASSERT(beta >= 1, "");
            HE_FMT_ASSERT(beta < 64, "");

            const uint64_t r = UMul96Lower64(twof, cache);
            return {
                ((r >> (64 - beta)) & 1) != 0,
                static_cast<uint32_t>(r >> (32 - beta)) == 0,
            };
        }

        static Uint ComputeLeftEndpointForShorterIntervalCase(const EntryType& cache, int32_t beta) noexcept
        {
            const EntryType value = (cache - (cache >> (Limits<float>::SignificandBits + 2)));
            const uint32_t shift = (64 - Limits<float>::SignificandBits - 1 - beta);
            return static_cast<Uint>(value >> shift);
        }

        static Uint ComputeRightEndpointForShorterIntervalCase(const EntryType& cache, int32_t beta) noexcept
        {
            const EntryType value = (cache + (cache >> (Limits<float>::SignificandBits + 1)));
            const uint32_t shift = (64 - Limits<float>::SignificandBits - 1 - beta);
            return static_cast<Uint>(value >> shift);
        }

        static Uint ComputeRoundUpForShorterIntervalCase(const EntryType& cache, int32_t beta) noexcept
        {
            return (static_cast<Uint>(cache >> (64 - Limits<float>::SignificandBits - 2 - beta)) + 1) / 2;
        }
    };

    template <>
    struct CacheAccessor<double>
    {
        using Uint = CarrierUint<double>;
        using EntryType = uint128_t;
        static constexpr int32_t MinK = -292;
        static constexpr int32_t MaxK = 341;
        static constexpr int32_t Kappa = 2;
        static constexpr int32_t BigDivisor = 1000;
        static constexpr int32_t SmallDivisor = 100;
        static constexpr int32_t ShorterIntervalTieLowerThreshold = -77;
        static constexpr int32_t ShorterIntervalTieUpperThreshold = -77;

        static EntryType GetCachedPower(int32_t k) noexcept;

        static uint32_t ComputeDelta(const EntryType& cache, int32_t beta) noexcept
        {
            return static_cast<uint32_t>(HE_UINT128_HIGH64(cache) >> (64 - 1 - beta));
        }

        struct ComputeMulResult
        {
            Uint result;
            bool isInteger;
        };

        static ComputeMulResult ComputeMul(Uint u, const EntryType& cache) noexcept
        {
            const uint128_t r = UMul192Upper128(u, cache);
            return { HE_UINT128_HIGH64(r), HE_UINT128_LOW64(r) == 0 };
        }

        struct ComputeMulParityResult
        {
            bool parity;
            bool isInteger;
        };
        static ComputeMulParityResult ComputeMulParity(Uint twof, const EntryType& cache, int32_t beta) noexcept
        {
            HE_FMT_ASSERT(beta >= 1, "");
            HE_FMT_ASSERT(beta < 64, "");

            const uint128_t r = UMul192Lower128(twof, cache);
            return {
                ((HE_UINT128_HIGH64(r) >> (64 - beta)) & 1) != 0,
                ((HE_UINT128_HIGH64(r) << beta) | (HE_UINT128_LOW64(r) >> (64 - beta))) == 0,
            };
        }

        static Uint ComputeLeftEndpointForShorterIntervalCase(const EntryType& cache, int beta) noexcept
        {
            const uint32_t shift = (64 - Limits<double>::SignificandBits - 1 - beta);
            return (HE_UINT128_HIGH64(cache) - (HE_UINT128_HIGH64(cache) >> (Limits<double>::SignificandBits + 2))) >> shift;
        }

        static Uint ComputeRightEndpointForShorterIntervalCase(const EntryType& cache, int beta) noexcept
        {
            const uint32_t shift = (64 - Limits<double>::SignificandBits - 1 - beta);
            return (HE_UINT128_HIGH64(cache) + (HE_UINT128_HIGH64(cache) >> (Limits<double>::SignificandBits + 1))) >> shift;
        }

        static Uint ComputeRoundUpForShorterIntervalCase(const EntryType& cache, int beta) noexcept
        {
            const uint32_t shift = (64 - Limits<double>::SignificandBits - 2 - beta);
            return ((HE_UINT128_HIGH64(cache) >> shift) + 1) / 2;
        }
    };

    template <typename T>
    bool IsLeftEndpointIntegerShorterInterval(int32_t exponent) noexcept
    {
        constexpr int32_t LeftEndpointLowerThreshold = 2;
        constexpr int32_t LeftEndpointUpperThreshold = 3;
        return exponent >= LeftEndpointLowerThreshold && exponent <= LeftEndpointUpperThreshold;
    }

    // Remove trailing zeros from n and return the number of zeros removed (float)
    inline int32_t RemoveTrailingZeros(uint32_t& n, int32_t s = 0) noexcept
    {
        HE_FMT_ASSERT(n != 0, "");
        // Modular inverse of 5 (mod 2^32): (ModInv5 * 5) mod 2^32 = 1.
        constexpr uint32_t ModInv5 = 0xcccccccd;
        constexpr uint32_t ModInv25 = 0xc28f5c29;  // = ModInv5 * ModInv5

        while (true)
        {
            const uint32_t q = Rotr32(n * ModInv25, 2);
            if (q > Limits<uint32_t>::Max / 100)
                break;

            n = q;
            s += 2;
        }

        const uint32_t q = Rotr32(n * ModInv5, 1);
        if (q <= Limits<uint32_t>::Max / 10)
        {
            n = q;
            s |= 1;
        }

        return s;
    }

    // Removes trailing zeros and returns the number of zeros removed (double)
    inline int32_t RemoveTrailingZeros(uint64_t& n) noexcept
    {
        HE_FMT_ASSERT(n != 0, "");

        // This magic number is ceil(2^90 / 10^8).
        constexpr uint64_t MagicNumber = 12379400392853802749ull;
        const uint128_t nm = UMul128(n, MagicNumber);

        // Is n is divisible by 10^8?
        if ((HE_UINT128_HIGH64(nm) & ((1ull << (90 - 64)) - 1)) == 0 && HE_UINT128_LOW64(nm) < MagicNumber)
        {
            // If yes, work with the quotient...
            uint32_t n32 = static_cast<uint32_t>(HE_UINT128_HIGH64(nm) >> (90 - 64));
            // ... and use the 32 bit variant of the function
            const int32_t s = RemoveTrailingZeros(n32, 8);
            n = n32;
            return s;
        }

        // If n is not divisible by 10^8, work with n itself.
        constexpr uint64_t ModInv5 = 0xcccccccccccccccd;
        constexpr uint64_t ModInv25 = 0x8f5c28f5c28f5c29;  // ModInv5 * ModInv5

        int s = 0;
        while (true)
        {
            const uint64_t q = Rotr64(n * ModInv25, 2);
            if (q > Limits<uint64_t>::Max / 100)
                break;

            n = q;
            s += 2;
        }

        const uint64_t q = Rotr64(n * ModInv5, 1);
        if (q <= Limits<uint64_t>::Max / 10)
        {
            n = q;
            s |= 1;
        }

        return s;
    }

    template <typename T>
    struct DecimalFP
    {
        using SignificandType = CarrierUint<T>;
        SignificandType significand;
        int32_t exponent;
    };

    // The main algorithm for shorter interval case
    template <typename T>
    inline DecimalFP<T> ShorterIntervalCase(int32_t exponent) noexcept
    {
        DecimalFP<T> ret;
        // Compute k and beta
        const int minus_k = FloorLog10Pow2MinusLog10Of4Over3(exponent);
        const int beta = exponent + FloorLog2Pow10(-minus_k);

        // Compute xi and zi
        using CacheType = CacheAccessor<T>;
        using CacheUint = typename CacheType::Uint;
        using CacheEntryType = typename CacheType::EntryType;
        const CacheEntryType cache = CacheType::GetCachedPower(-minus_k);

        CacheUint xi = CacheType::ComputeLeftEndpointForShorterIntervalCase(cache, beta);
        const CacheUint zi = CacheType::ComputeRightEndpointForShorterIntervalCase(cache, beta);

        // If the left endpoint is not an integer, increase it
        if (!IsLeftEndpointIntegerShorterInterval<T>(exponent))
            ++xi;

        // Try bigger divisor
        ret.significand = zi / 10;

        // If succeed, remove trailing zeros if necessary and return
        if (ret.significand * 10 >= xi)
        {
            ret.exponent = minus_k + 1;
            ret.exponent += RemoveTrailingZeros(ret.significand);
            return ret;
        }

        // Otherwise, compute the round-up of y
        ret.significand = CacheType::ComputeRoundUpForShorterIntervalCase(cache, beta);
        ret.exponent = minus_k;

        // When tie occurs, choose one of them according to the rule
        if (exponent >= CacheType::ShorterIntervalTieLowerThreshold
            && exponent <= CacheType::ShorterIntervalTieUpperThreshold)
        {
            ret.significand = ret.significand % 2 == 0 ? ret.significand : ret.significand - 1;
        }
        else if (ret.significand < xi)
        {
            ++ret.significand;
        }
        return ret;
    }

    template <typename T>
    inline DecimalFP<T> ToDecimal(T x) noexcept
    {
        // Step 1: integer promotion & Schubfach multiplier calculation.
        using Info = Limits<T>;
        using Uint = CarrierUint<T>;
        using CacheType = CacheAccessor<T>;
        using CacheEntryType = typename CacheType::EntryType;

        constexpr Uint ImplicitBit = Uint(1) << Info::SignificandBits;
        constexpr Uint SignificandMask = ImplicitBit - 1;
        constexpr Uint ExponentMask = ((Uint(1) << Info::ExponentBits) - 1) << Info::SignificandBits;
        constexpr Uint ExponentBias = Info::MaxExponent - 1;

        const Uint bits = BitCast<Uint>(x);

        // Extract significand bits and exponent bits.
        Uint significand = (bits & SignificandMask);
        int32_t exponent = static_cast<int32_t>((bits & ExponentMask) >> Info::SignificandBits);

        // Check if normal.
        if (exponent != 0)
        {
            exponent -= ExponentBias + Info::SignificandBits;

            // Shorter interval case; proceed like Schubfach.
            // In fact, when exponent == 1 and significand == 0, the interval is
            // regular. However, it can be shown that the end-results are anyway same.
            if (significand == 0)
                return ShorterIntervalCase<T>(exponent);

            significand |= (Uint(1) << Info::SignificandBits);
        }
        else
        {
            // Subnormal case; the interval is always regular.
            if (significand == 0)
                return { 0, 0 };

            exponent = Info::MinExponent - static_cast<int32_t>(Info::SignificandBits) - 1;
        }

        const bool includeLeftEndpoint = (significand % 2 == 0);
        const bool includeRightEndpoint = includeLeftEndpoint;

        // Compute k and beta.
        const int32_t minusK = FloorLog10Pow2(exponent) - CacheType::Kappa;
        const CacheEntryType cache = CacheType::GetCachedPower(-minusK);
        const int32_t beta = exponent + FloorLog2Pow10(-minusK);

        // Compute zi and deltai.
        // 10^kappa <= deltai < 10^(kappa + 1)
        const uint32_t deltai = CacheType::ComputeDelta(cache, beta);
        const Uint twofc = significand << 1;

        // For the case of binary32, the result of integer check is not correct for
        // 29711844 * 2^-82
        // = 6.1442653300000000008655037797566933477355632930994033813476... * 10^-18
        // and 29711844 * 2^-81
        // = 1.2288530660000000001731007559513386695471126586198806762695... * 10^-17,
        // and they are the unique counterexamples. However, since 29711844 is even,
        // this does not cause any problem for the endpoints calculations; it can only
        // cause a problem when we need to perform integer check for the center.
        // Fortunately, with these inputs, that branch is never executed, so we are
        // fine.
        const auto mulZ = CacheType::ComputeMul((twofc | 1) << beta, cache);

        // Step 2: Try larger divisor; remove trailing zeros if necessary.

        // Using an upper bound on zi, we might be able to optimize the division
        // better than the compiler; we are computing zi / BigDivisor here.
        DecimalFP<T> ret;
        ret.significand = DivideBy10ToKappaPlus1(mulZ.result);
        uint32_t r = static_cast<uint32_t>(mulZ.result - CacheType::BigDivisor * ret.significand);

        if (r < deltai)
        {
            // Exclude the right endpoint if necessary.
            if (r == 0 && (mulZ.isInteger & !includeRightEndpoint))
            {
                --ret.significand;
                r = CacheType::BigDivisor;
                goto small_divisor_case_label;
            }
        }
        else if (r > deltai)
        {
            goto small_divisor_case_label;
        }
        else
        {
            // r == deltai; compare fractional parts.
            const auto mulX = CacheType::ComputeMulParity(twofc - 1, cache, beta);

            if (!(mulX.parity | (mulX.isInteger & includeLeftEndpoint)))
                goto small_divisor_case_label;
        }
        ret.exponent = minusK + CacheType::Kappa + 1;

        // We may need to remove trailing zeros.
        ret.exponent += RemoveTrailingZeros(ret.significand);
        return ret;

        // Step 3: Find the significand with the smaller divisor.

    small_divisor_case_label:
        ret.significand *= 10;
        ret.exponent = minusK + CacheType::Kappa;

        uint32_t dist = r - (deltai / 2) + (CacheType::SmallDivisor / 2);
        const bool approxParityY = ((dist ^ (CacheType::SmallDivisor / 2)) & 1) != 0;

        // Is dist divisible by 10^kappa?
        const bool divisibleBySmallDivisor = CheckDivisibilityAndDivideByPow10<CacheType::Kappa>(dist);

        // Add dist / 10^kappa to the significand.
        ret.significand += dist;

        if (!divisibleBySmallDivisor)
            return ret;

        // Check z^(f) >= epsilon^(f).
        // We have either yi == zi - epsiloni or yi == (zi - epsiloni) - 1,
        // where yi == zi - epsiloni if and only if z^(f) >= epsilon^(f).
        // Since there are only 2 possibilities, we only need to care about the
        // parity. Also, zi and r should have the same parity since the divisor
        // is an even number.
        const auto mulY = CacheType::ComputeMulParity(twofc, cache, beta);

        // If z^(f) >= epsilon^(f), we might have a tie when z^(f) == epsilon^(f),
        // or equivalently, when y is an integer.
        if (mulY.parity != approxParityY)
            --ret.significand;
        else if (mulY.isInteger & (ret.significand % 2 != 0))
            --ret.significand;

        return ret;
    }
}
