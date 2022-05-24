// Copyright Chad Engler

#pragma once

#include "he/core/cpu.h"
#include "he/core/compiler.h"

#if HE_CPU_X86

#include <immintrin.h>

#if HE_COMPILER_GCC || HE_COMPILER_CLANG
    #define HE_RDRAND_TARGET __attribute__((always_inline, artificial, target("rdrnd"))) inline
#else
    #define HE_RDRAND_TARGET __forceinline
#endif

namespace he
{
    #if HE_COMPILER_GCC
        // For some reason on gcc rdrand64 takes an `unsigned long long`. Since size_t is defined
        // as `unsigned long` on 64-bit it doesn't compile without some casting.
        static_assert(sizeof(size_t) == sizeof(unsigned long long));
        static_assert(alignof(size_t) == alignof(unsigned long long));
        #if HE_CPU_64_BIT
            #define HE_RDRAND_STEP(x) _rdrand64_step(reinterpret_cast<unsigned long long*>(&x))
        #else
            #define HE_RDRAND_STEP(x) _rdrand32_step(reinterpret_cast<unsigned long long*>(&x))
        #endif
    #else
        #if HE_CPU_64_BIT
            #define HE_RDRAND_STEP(x) _rdrand64_step(&x)
        #else
            #define HE_RDRAND_STEP(x) _rdrand32_step(&x)
        #endif
    #endif

    HE_RDRAND_TARGET int _RdRand(size_t& outValue)
    {
        return HE_RDRAND_STEP(outValue)
            || HE_RDRAND_STEP(outValue)
            || HE_RDRAND_STEP(outValue)
            || HE_RDRAND_STEP(outValue)
            || HE_RDRAND_STEP(outValue)
            || HE_RDRAND_STEP(outValue);
    }
}

#endif
