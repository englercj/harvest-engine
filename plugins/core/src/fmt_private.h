// Copyright Chad Engler

#pragma once

#include "he/core/config.h"
#include "he/core/concepts.h"
#include "he/core/types.h"

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

#if defined(__INT128_TYPE__)
    #define HE_HAS_UINT128 1
    #define HE_UINT128_HIGH64(x) (static_cast<uint64_t>((x) >> 64))
    #define HE_UINT128_LOW64(x) (static_cast<uint64_t>(x))
    using uint128_t = unsigned __INT128_TYPE__;
#elif defined(__SIZEOF_INT128__)
    #define HE_HAS_UINT128 1
    #define HE_UINT128_HIGH64(x) (static_cast<uint64_t>((x) >> 64))
    #define HE_UINT128_LOW64(x) (static_cast<uint64_t>(x))
    using uint128_t = unsigned __int128;
#else
    #define HE_HAS_UINT128 0
    #define HE_UINT128_HIGH64(x) ((x).High())
    #define HE_UINT128_LOW64(x) ((x).Low())

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
}
