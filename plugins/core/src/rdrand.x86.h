// Copyright Chad Engler

#pragma once

#include "he/core/cpu.h"
#include "he/core/compiler.h"

#if HE_CPU_X86

#include <immintrin.h>

#if HE_COMPILER_GCC || HE_COMPILER_CLANG
    #define HE_RDRAND_TARGET __attribute__((__always_inline__, __nodebug__, __target__("rdrnd"))) inline
#else
    #define HE_RDRAND_TARGET __forceinline
#endif

namespace he
{
    #if HE_CPU_64_BIT
        #define HE_RDRAND_STEP(p) _rdrand64_step(p)
    #else
        #define HE_RDRAND_STEP(p) _rdrand32_step(p)
    #endif

    HE_RDRAND_TARGET int _RdRand(size_t& outValue)
    {
        return HE_RDRAND_STEP(&outValue)
            || HE_RDRAND_STEP(&outValue)
            || HE_RDRAND_STEP(&outValue)
            || HE_RDRAND_STEP(&outValue)
            || HE_RDRAND_STEP(&outValue)
            || HE_RDRAND_STEP(&outValue);
    }
}

#endif
