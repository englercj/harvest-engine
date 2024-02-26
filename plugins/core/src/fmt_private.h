// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/concepts.h"
#include "he/core/limits.h"
#include "he/core/string.h"
#include "he/core/types.h"

#if HE_COMPILER_MSVC
    extern "C" unsigned char _BitScanReverse(unsigned long* Index, unsigned long Mask);
    #pragma intrinsic(_BitScanReverse)

    #if HE_CPU_64_BIT
        extern "C" unsigned char _BitScanReverse64(unsigned long* Index, unsigned __int64 Mask);
        #pragma intrinsic(_BitScanReverse64)
    #endif
#endif

// Can't use the normal assert flow, because it could come back into Fmt again.
#if defined(_PREFAST_)
    #define HE_FMT_ASSERT(expr, msg) __assume(expr)
#elif !HE_ENABLE_ASSERTIONS
    #define HE_FMT_ASSERT(expr, msg)
#else
    #define HE_FMT_ASSERT(expr, msg) (HE_LIKELY(expr) ? void(0) : FmtError(msg))
#endif

namespace he
{
    [[noreturn]] void FmtError(const char* msg);

#if HE_HAS_INT128
    #define HE_UINT128_HIGH64(x) (static_cast<uint64_t>((x) >> 64))
    #define HE_UINT128_LOW64(x) (static_cast<uint64_t>(x))
    #define HE_UINT128_CONST(high, low) ((static_cast<uint128_t>(high) << 64) | low)
#else
    #define HE_UINT128_HIGH64(x) ((x).High())
    #define HE_UINT128_LOW64(x) ((x).Low())
    #define HE_UINT128_CONST(high, low) uint128_t(high, low)

    #if HE_COMPILER_MSVC
        extern "C" unsigned char _addcarry_u64(
            unsigned char Carry,
            unsigned __int64 Source1,
            unsigned __int64 Source2,
            unsigned __int64* Destination);
        #pragma intrinsic(_addcarry_u64)
    #endif
    class uint128_t
    {
    private:
        uint64_t m_lo, m_hi;

    public:
        constexpr uint128_t(uint64_t hi, uint64_t lo) : m_lo(lo), m_hi(hi) {}
        constexpr uint128_t(uint64_t value = 0) : m_lo(value), m_hi(0) {}

        constexpr uint64_t High() const noexcept { return m_hi; }
        constexpr uint64_t Low() const noexcept { return m_lo; }

        template <Integral T>
        constexpr explicit operator T() const { return static_cast<T>(m_lo); }

        constexpr bool operator==(const uint128_t& x) const noexcept { return m_hi == x.m_hi && m_lo == x.m_lo; }
        constexpr bool operator!=(const uint128_t& x) const noexcept { return m_hi != x.m_hi || m_lo != x.m_lo; }
        constexpr bool operator>(const uint128_t& x) const noexcept { return m_hi != x.m_hi ? m_hi > x.m_hi : m_lo > x.m_lo; }

        constexpr uint128_t operator|(const uint128_t& x) const noexcept { return { m_hi | x.m_hi, m_lo | x.m_lo }; }
        constexpr uint128_t operator&(const uint128_t& x) const noexcept { return { m_hi & x.m_hi, m_lo & x.m_lo }; }
        friend constexpr uint128_t operator~(const uint128_t& x) noexcept { return { ~x.m_hi, ~x.m_lo }; }

        uint128_t operator+(const uint128_t& x) const noexcept { uint128_t result = *this; result += x; return result; }
        uint128_t operator-(uint64_t x) const noexcept { return { m_hi - (m_lo < x ? 1 : 0), m_lo - x }; }
        uint128_t operator*(uint32_t x) const noexcept
        {
            HE_FMT_ASSERT(m_hi == 0, "");
            uint64_t hi = (m_lo >> 32) * x;
            uint64_t lo = (m_lo & ~uint32_t()) * x;
            uint64_t new_lo = (hi << 32) + lo;
            return { (hi >> 32) + (new_lo < lo ? 1 : 0), new_lo };
        }

        constexpr uint128_t operator>>(int shift) const noexcept
        {
            if (shift == 64) return {0, m_hi};
            if (shift > 64) return uint128_t(0, m_hi) >> (shift - 64);
            return { m_hi >> shift, (m_hi << (64 - shift)) | (m_lo >> shift) };
        }

        constexpr uint128_t operator<<(int shift) const noexcept
        {
            if (shift == 64) return {m_lo, 0};
            if (shift > 64) return uint128_t(m_lo, 0) << (shift - 64);
            return { m_hi << shift | (m_lo >> (64 - shift)), (m_lo << shift) };
        }

        constexpr uint128_t& operator<<=(int shift) noexcept { return *this = *this << shift; }
        constexpr uint128_t& operator>>=(int shift) noexcept { return *this = *this >> shift; }
        constexpr void operator&=(uint128_t x) noexcept { m_lo &= x.m_lo; m_hi &= x.m_hi; }

        constexpr void operator+=(uint128_t x) noexcept
        {
            uint64_t new_lo = m_lo + x.m_lo;
            uint64_t new_hi = m_hi + x.m_hi + (new_lo < m_lo ? 1 : 0);
            HE_FMT_ASSERT(new_hi >= m_hi, "");
            m_lo = new_lo;
            m_hi = new_hi;
        }

        constexpr uint128_t& operator+=(uint64_t x) noexcept
        {
            if (__builtin_is_constant_evaluated())
            {
                m_lo += x;
                m_hi += (m_lo < x ? 1 : 0);
                return *this;
            }
        #if HE_HAS_BUILTIN(__builtin_addcll) && !defined(__ibmxl__)
            unsigned long long carry;
            m_lo = __builtin_addcll(m_lo, x, 0, &carry);
            m_hi += carry;
        #elif HE_HAS_BUILTIN(__builtin_ia32_addcarryx_u64) && !defined(__ibmxl__)
            unsigned long long result;
            auto carry = __builtin_ia32_addcarryx_u64(0, m_lo, x, &result);
            m_lo = result;
            m_hi += carry;
        #elif HE_COMPILER_MSVC && HE_CPU_X86_64
            const unsigned char carry = _addcarry_u64(0, m_lo, x, &m_lo);
            _addcarry_u64(carry, m_hi, 0, &m_hi);
        #else
            m_lo += x;
            m_hi += (m_lo < x ? 1 : 0);
        #endif
            return *this;
        }
    };
#endif

    template <typename T>
    using CarrierUint = Conditional<sizeof(T) <= 4, uint32_t, Conditional<sizeof(T) <= 8, uint64_t, uint128_t>>;

    // Converts value in the range [0, 100) to a string.
    constexpr const char* Digits2(uint64_t value)
    {
        return &"0001020304050607080910111213141516171819"
            "2021222324252627282930313233343536373839"
            "4041424344454647484950515253545556575859"
            "6061626364656667686970717273747576777879"
            "8081828384858687888990919293949596979899"[value * 2];
    };

    HE_FORCE_INLINE uint32_t CountLeadingZeroes(uint32_t x)
    {
    #if HE_COMPILER_MSVC
        unsigned long r = 0;
        _BitScanReverse(&r, x);
        return 31 ^ r;
    #else
        return static_cast<uint32_t>(__builtin_clz(x));
    #endif
    }

    HE_FORCE_INLINE uint32_t CountLeadingZeroes(uint64_t x)
    {
    #if HE_COMPILER_MSVC
        unsigned long r = 0;
    #if HE_CPU_64_BIT
        _BitScanReverse64(&r, x);
    #else
        if (_BitScanReverse(&r, static_cast<uint32_t>(x >> 32)))
            return 63 ^ (r + 32);
        _BitScanReverse(&r, static_cast<uint32_t>(x));
    #endif
        return 63 ^ r;
    #else
        return static_cast<uint32_t>(__builtin_clzll(x));
    #endif
    }

    extern const uint64_t DigitCountLookup[32];
    extern const uint64_t ZeroOrPowersOf10[21];
    extern const uint8_t Bsr2Log10[64];

    HE_FORCE_INLINE uint32_t CountDigits(uint64_t x)
    {
        const uint8_t t = Bsr2Log10[CountLeadingZeroes(x | 1) ^ 63];
        return t - (x < ZeroOrPowersOf10[t]);
    }

    HE_FORCE_INLINE uint32_t CountDigits(uint32_t x)
    {
        const uint64_t inc = DigitCountLookup[CountLeadingZeroes(x | 1) ^ 31];
        return static_cast<uint32_t>((x + inc) >> 32);
    }

    template <uint32_t Bits, typename T>
    inline uint32_t CountDigits(T x)
    {
        if constexpr (Limits<T>::ValueBits == 32)
            return (CountLeadingZeroes(static_cast<uint32_t>(x) | 1) ^ 31) / Bits + 1;

        uint32_t count = 0;
        do
        {
            ++count;
        } while ((x >>= Bits) != 0);
        return count;
    }

    inline char* FmtResize(String& out, uint32_t len)
    {
        const uint32_t size = out.Size();
        out.Resize(size + len, DefaultInit);
        return out.Data() + size;
    }

    struct FormatDecimalResult
    {
        char* begin;
        char* end;
    };

    template <UnsignedIntegral T>
    static FormatDecimalResult FormatDecimal(char* it, T value, uint32_t len)
    {
        HE_FMT_ASSERT(len >= CountDigits(value), "Invalid digit count");

        it += len;
        char* end = it;
        while (value >= 100)
        {
            // Integer division is slow so do it for a group of two digits instead
            // of for every digit. The idea comes from the talk by Alexandrescu
            // "Three Optimization Tips for C++". See speed-test for a comparison.
            it -= 2;
            MemCopy(it, Digits2(value % 100), 2);
            value /= 100;
        }

        if (value < 10)
        {
            *--it = static_cast<char>('0' + value);
            return { it, end };
        }

        it -= 2;
        MemCopy(it, Digits2(value), 2);
        return { it, end };
    }
}
