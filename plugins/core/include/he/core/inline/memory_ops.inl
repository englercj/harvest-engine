// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/cpu_info.h"
#include "he/core/simd.h"
#include "he/core/types.h"

// ------------------------------------------------------------------------------------------------
#if HE_COMPILER_MSVC
    extern "C" unsigned char _BitScanReverse(unsigned long*, unsigned long);
    extern "C" unsigned char _BitScanForward(unsigned long*, unsigned long);
    #pragma intrinsic(_BitScanReverse, _BitScanForward)

#if HE_CPU_64_BIT
    extern "C" unsigned char _BitScanReverse64(unsigned long*, unsigned __int64);
    extern "C" unsigned char _BitScanForward64(unsigned long*, unsigned __int64);
    #pragma intrinsic(_BitScanReverse64, _BitScanForward64)
#endif

#if HE_CPU_X86
    extern "C" unsigned int __lzcnt(unsigned int);
    extern "C" unsigned int _tzcnt_u32(unsigned int);
    extern "C" unsigned int __popcnt(unsigned int);
    #pragma intrinsic(__lzcnt, _tzcnt_u32, __popcnt)
#endif

#if HE_CPU_X86_64
    extern "C" unsigned __int64 __lzcnt64(unsigned __int64);
    extern "C" unsigned __int64 _tzcnt_u64(unsigned __int64);
    extern "C" unsigned __int64 __popcnt64(unsigned __int64);
    #pragma intrinsic(__lzcnt64, _tzcnt_u64, __popcnt64)
#endif

#if HE_CPU_ARM
    extern "C" unsigned int _CountLeadingZeros(unsigned long);
    extern "C" unsigned int _CountTrailingZeros(unsigned long);
    extern "C" unsigned int _CountLeadingZeros64(unsigned __int64);
    extern "C" unsigned int _CountTrailingZeros64(unsigned __int64);
    #pragma intrinsic(_CountLeadingZeros, _CountTrailingZeros)
    #pragma intrinsic(_CountLeadingZeros64, _CountTrailingZeros64)
#endif
#endif

namespace he
{
#if HE_COMPILER_MSVC
    // --------------------------------------------------------------------------------------------
    HE_FORCE_INLINE uint32_t _Clz32_Bsr(uint32_t x)
    {
        unsigned long r = 0;
        if (!_BitScanReverse(&r, x))
            return 32;
        return static_cast<uint32_t>(31 - r);
    }

    HE_FORCE_INLINE uint32_t _Clz64_Bsr(uint64_t x)
    {
        unsigned long r = 0;
    #if HE_CPU_64_BIT
        if (!_BitScanReverse64(&r, x))
            return 64;
    #else
        const uint32_t high = static_cast<uint32_t>(x >> 32);
        if (_BitScanReverse(&r, high))
            return static_cast<uint32_t>(31 - r);

        const uint32_t low = static_cast<uint32_t>(x);
        if (!_BitScanReverse(&r, low))
            return 64;
    #endif
        return static_cast<uint32_t>(63 - r);
    }

    HE_FORCE_INLINE uint32_t _Crz32_Bsf(uint32_t x)
    {
        unsigned long r = 0;
        if (!_BitScanForward(&r, x))
            return 32;
        return static_cast<uint32_t>(r);
    }

    HE_FORCE_INLINE uint32_t _Crz64_Bsf(uint64_t x)
    {
        unsigned long r = 0;
    #if HE_CPU_64_BIT
        if (!_BitScanForward64(&r, x))
            return 64;

        return static_cast<uint32_t>(r);
    #else
        const uint32_t low = static_cast<uint32_t>(x);
        if (_BitScanForward(&r, low))
            return static_cast<uint32_t>(r);

        const uint32_t high = static_cast<uint32_t>(x >> 32);
        if (!_BitScanForward(&r, high))
            return 64;

        return static_cast<uint32_t>(r + 32);
    #endif
    }

#if HE_CPU_X86
    // --------------------------------------------------------------------------------------------
    HE_FORCE_INLINE uint32_t _Clz32_Lzcnt(uint64_t x)
    {
        return static_cast<uint32_t>(__lzcnt(x));
    }

    HE_FORCE_INLINE uint32_t _Clz64_Lzcnt(uint64_t x)
    {
    #if HE_CPU_X86_64
        return static_cast<uint32_t>(__lzcnt64(x));
    #else
        const uint32_t high = static_cast<uint32_t>(x >> 32);
        const uint32_t low = static_cast<uint32_t>(x);
        return high == 0 ? (32 + _Clz32_Lzcnt(low)) : _Clz32_Lzcnt(high);
    #endif
    }

    HE_FORCE_INLINE uint32_t _Crz32_Tzcnt(uint64_t x)
    {
        return static_cast<uint32_t>(_tzcnt_u32(x));
    }

    HE_FORCE_INLINE uint32_t _Crz64_Tzcnt(uint64_t x)
    {
    #if HE_CPU_X86_64
        return static_cast<uint32_t>(_tzcnt_u64(x));
    #else
        const uint32_t high = static_cast<uint32_t>(x >> 32);
        const uint32_t low = static_cast<uint32_t>(x);
        return low == 0 ? (32 + _Crz32_Tzcnt(high)) : _Crz32_Tzcnt(low);
    #endif
    }
#endif

    // --------------------------------------------------------------------------------------------
    HE_FORCE_INLINE uint32_t _PopCnt32_Fallback(uint32_t x)
    {
        // we static_cast these bit patterns in order to truncate them to the correct size
        x = static_cast<uint32_t>(x - ((x >> 1) & static_cast<uint32_t>(0x5555'5555'5555'5555ull)));
        x = static_cast<uint32_t>((x & static_cast<uint32_t>(0x3333'3333'3333'3333ull)) + ((x >> 2) & static_cast<uint32_t>(0x3333'3333'3333'3333ull)));
        x = static_cast<uint32_t>((x + (x >> 4)) & static_cast<uint32_t>(0x0F0F'0F0F'0F0F'0F0Full));

        // Multiply by one in each byte, so that it will have the sum of all source bytes in the highest byte
        x = static_cast<uint32_t>(x * static_cast<uint32_t>(0x0101'0101'0101'0101ull));

        // Extract highest byte
        return static_cast<uint32_t>(x >> (32 - 8));
    }

    HE_FORCE_INLINE uint32_t _PopCnt64_Fallback(uint32_t x)
    {
        const uint32_t high = static_cast<uint32_t>(x >> 32);
        const uint32_t low = static_cast<uint32_t>(x);
        return _PopCnt32_Fallback(high) + _PopCnt32_Fallback(low);
    }

#if HE_CPU_X86
    HE_FORCE_INLINE uint32_t _PopCnt32_x86(uint64_t x)
    {
        return __popcnt(x);
    }

    HE_FORCE_INLINE uint32_t _PopCnt64_x86(uint64_t x)
    {
    #if HE_CPU_X86_64
        return __popcnt64(x);
    #else
        const uint32_t high = static_cast<uint32_t>(x >> 32);
        const uint32_t low = static_cast<uint32_t>(x);
        return _PopCnt32_x86(high) + _PopCnt32_x86(low);
    #endif
    }
#endif

#if HE_CPU_ARM
    HE_FORCE_INLINE uint32_t _PopCnt32_ARM(uint32_t x)
    {
    #if HE_CPU_ARM_64
        const uint64_t xu64 = static_cast<uint64_t>(x);
        const __n64 tmp = neon_cnt(__uint64ToN64_v(xu64));
        return static_cast<uint32_t>(neon_addv8(tmp).n8_i8[0]);
    #else
        return _PopCnt32_Fallback(x);
    #endif
    }

    HE_FORCE_INLINE uint32_t _PopCnt64_ARM(uint64_t x)
    {
    #if HE_CPU_ARM_64
        const __n64 tmp = neon_cnt(__uint64ToN64_v(x));
        return static_cast<uint32_t>(neon_addv8(tmp).n8_i8[0]);
    #else
        return _PopCnt64_Fallback(x);
    #endif
    }
#endif
#endif

    // --------------------------------------------------------------------------------------------
    HE_FORCE_INLINE uint32_t CountLeadingZeros(uint32_t x)
    {
    #if HE_HAS_BUILTIN(__builtin_clz)
        return static_cast<uint32_t>(__builtin_clz(x));
    #elif HE_COMPILER_MSVC
        #if HE_CPU_X86 && HE_SIMD_AVX2
            return _Clz32_Lzcnt(x);
        #elif HE_CPU_X86
            const CpuInfo& info = GetCpuInfo();
            if (info.x86.avx2)
                return _Clz32_Lzcnt(x);
            else
                return _Clz32_Bsr(x);
        #elif HE_CPU_ARM
            return _CountLeadingZeros(x);
        #else
            return _Clz32_Bsr(x);
        #endif
    #else
        #error "No clz32 implementation"
    #endif
    }

    HE_FORCE_INLINE uint32_t CountLeadingZeros(uint64_t x)
    {
    #if HE_HAS_BUILTIN(__builtin_clzll)
        return static_cast<uint32_t>(__builtin_clzll(x));
    #elif HE_COMPILER_MSVC
        #if HE_CPU_X86 && HE_SIMD_AVX2
            return _Clz64_Lzcnt(x);
        #elif HE_CPU_X86
            const CpuInfo& info = GetCpuInfo();
            if (info.x86.avx2)
                return _Clz64_Lzcnt(x);
            else
                return _Clz64_Bsr(x);
        #elif HE_CPU_ARM
            return _CountLeadingZeros64(x);
        #else
            return _Clz64_Bsr(x);
        #endif
    #else
        #error "No clz64 implementation"
    #endif
    }

    // --------------------------------------------------------------------------------------------
    HE_FORCE_INLINE uint32_t CountTrailingZeros(uint32_t x)
    {
    #if HE_HAS_BUILTIN(__builtin_ctz)
        return static_cast<uint32_t>(__builtin_ctz(x));
    #elif HE_COMPILER_MSVC
        #if HE_CPU_X86 && HE_SIMD_AVX2
            return _Crz32_Tzcnt(x);
        #elif HE_CPU_X86
            const CpuInfo& info = GetCpuInfo();
            if (info.x86.avx2)
                return _Crz32_Tzcnt(x);
            else
                return _Crz32_Bsf(x);
        #elif HE_CPU_ARM
            return _CountTrailingZeroes(x);
        #else
            return _Crz32_Bsf(x);
        #endif
    #else
        #error "No ctz32 implementation"
    #endif
    }

    HE_FORCE_INLINE uint32_t CountTrailingZeros(uint64_t x)
    {
    #if HE_HAS_BUILTIN(__builtin_ctzll)
        return static_cast<uint32_t>(__builtin_ctzll(x));
    #elif HE_COMPILER_MSVC
        #if HE_CPU_X86 && HE_SIMD_AVX2
            return _Crz64_Tzcnt(x);
        #elif HE_CPU_X86
            const CpuInfo& info = GetCpuInfo();
            if (info.x86.avx2)
                return _Crz64_Tzcnt(x);
            else
                return _Crz64_Bsf(x);
        #elif HE_CPU_ARM
            return _CountTrailingZeroes64(x);
        #else
            return _Crz64_Bsf(x);
        #endif
    #else
        #error "No ctz64 implementation"
    #endif
    }

    HE_FORCE_INLINE uint32_t PopCount(uint32_t x)
    {
    #if HE_HAS_BUILTIN(__builtin_popcount)
        return static_cast<uint32_t>(__builtin_popcount(x));
    #elif HE_COMPILER_MSVC
        #if HE_CPU_X86 && HE_SIMD_SSE4_2
            return _PopCnt32_x86(x);
        #elif HE_CPU_X86
            const CpuInfo& info = GetCpuInfo();
            if (info.x86.sse42)
                return _PopCnt32_x86(x);
            else
                return _PopCnt32_Fallback(x);
        #elif HE_CPU_ARM_64 && HE_SIMD_NEON
            return _PopCnt32_ARM(x);
        #else
            return _PopCnt32_Fallback(x);
        #endif
    #else
        #error "No popcnt32 implementation"
    #endif
    }

    HE_FORCE_INLINE uint32_t PopCount(uint64_t x)
    {
    #if HE_HAS_BUILTIN(__builtin_popcountll)
        return static_cast<uint32_t>(__builtin_popcountll(x));
    #elif HE_COMPILER_MSVC
        #if HE_CPU_X86 && HE_SIMD_SSE4_2
            return _PopCnt64_x86(x);
        #elif HE_CPU_X86
            const CpuInfo& info = GetCpuInfo();
            if (info.x86.sse42)
                return _PopCnt64_x86(x);
            else
                return _PopCnt64_Fallback(x);
        #elif HE_CPU_ARM_64 && HE_SIMD_NEON
            return _PopCnt64_ARM(x);
        #else
            return _PopCnt64_Fallback(x);
        #endif
    #else
        #error "No popcnt64 implementation"
    #endif
    }
}
