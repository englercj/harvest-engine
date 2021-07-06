#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"

#ifndef HE_SIMD_ENABLE
    #define HE_SIMD_ENABLE 1
#endif

#define HE_SIMD_AVX                 0
#define HE_SIMD_AVX2                0
#define HE_SIMD_NEON                0
#define HE_SIMD_SSE2                0
#define HE_SIMD_SSE3                0
#define HE_SIMD_SSE4_1              0
#define HE_SIMD_SSE4_2              0
#define HE_SIMD_FMA3                0

#if HE_SIMD_ENABLE

#if HE_CPU_X86 && (defined(__AVX__) || defined(__AVX2__))
    #include <immintrin.h>

    #undef HE_SIMD_AVX
    #define HE_SIMD_AVX             1

    #if defined(__AVX2__)
        #undef HE_SIMD_AVX2
        #define HE_SIMD_AVX2        1

        #undef HE_SIMD_FMA3
        #define HE_SIMD_FMA3        1
    #endif

    #undef HE_SIMD_SSE2
    #define HE_SIMD_SSE2            1

    #undef HE_SIMD_SSE3
    #define HE_SIMD_SSE3            1

    #undef HE_SIMD_SSE4_1
    #define HE_SIMD_SSE4_1          1

    #undef HE_SIMD_SSE4_2
    #define HE_SIMD_SSE4_2          1
#elif HE_CPU_X86 && (defined(__SSE2__) || HE_CPU_64_BIT || defined(_M_IX86_FP))
    #include <immintrin.h>

    #undef HE_SIMD_SSE2
    #define HE_SIMD_SSE2             1

    #if defined(__SSE3__)
        #undef HE_SIMD_SSE3
        #define HE_SIMD_SSE3        1
    #endif

    #if defined(__SSE4_1__)
        #undef HE_SIMD_SSE4_1
        #define HE_SIMD_SSE4_1      1
    #endif

    #if defined(__SSE4_2__)
        #undef HE_SIMD_SSE4_2
        #define HE_SIMD_SSE4_2      1
    #endif

    #if defined(__FMA__)
        #undef HE_SIMD_FMA3
        #define HE_SIMD_FMA3        1
    #endif
#elif HE_CPU_ARM && (defined(__ARM_NEON__) || defined(__ARM_NEON))
    #if HE_CPU_ARM_64 && HE_COMPILER_MSVC
        #include <arm64_neon.h>
    #else
        #include <arm_neon.h>
    #endif

    #undef  HE_SIMD_NEON
    #define HE_SIMD_NEON            1
#endif

#endif

#if HE_SIMD_SSE2
    using Simd128 = __m128;
#elif HE_SIMD_NEON
    using Simd128 = float32x4_t;
#else
    struct HE_ALIGNED(16) Simd128 { float x, y, z, w; };
#endif

constexpr Simd128 InitSimd128(float x, float y, float z, float w)
{
    #if HE_SIMD_SSE2 && HE_COMPILER_MSVC
        return Simd128{ .m128_f32 = { x, y, z, w } };
    #elif HE_SIMD_NEON && HE_COMPILER_MSVC
        return Simd128{ .n128_f32 = { x, y, z, w } };
    #else
        return Simd128{ x, y, z, w };
    #endif
}
