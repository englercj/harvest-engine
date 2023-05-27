// Copyright Chad Engler

#pragma once

#define HAVE_AVX        0
#define HAVE_AVX2       0
#define HAVE_AVX512     0
#define HAVE_NEON32     0
#define HAVE_NEON64     0
#define HAVE_SSSE3      0
#define HAVE_SSE41      0
#define HAVE_SSE42      0

#define HE_TB64_CPU_X86_32   (defined(_M_IX86) || defined(__i386__))
#define HE_TB64_CPU_X86_64   (defined(_M_AMD64) || defined(__x86_64__))
#define HE_TB64_CPU_ARM_32   (defined(_M_ARM) || defined(__arm__))
#define HE_TB64_CPU_ARM_64   (defined(_M_ARM64) || defined(__aarch64__))

#if (HE_TB64_CPU_X86_32 || HE_TB64_CPU_X86_64) && (defined(__AVX__) || defined(__AVX2__))
    #undef HAVE_AVX
    #define HAVE_AVX 1

    #if defined(__AVX2__)
        #undef HAVE_AVX2
        #define HAVE_AVX2 1
    #endif

    #if defined(__AVX512__)
        #undef HAVE_AVX512
        #define HAVE_AVX512 1
    #endif
#elif HE_TB64_CPU_X86_64 || (HE_TB64_CPU_X86_32 && (defined(__SSE2__) || defined(_M_IX86_FP)))
    #if defined(__SSSE3__)
        #undef HAVE_SSSE3
        #define HAVE_SSSE3 1
    #endif

    #if defined(__SSE4_1__)
        #undef HAVE_SSE41
        #define HAVE_SSE41 1
    #endif

    #if defined(__SSE4_2__)
        #undef HAVE_SSE42
        #define HAVE_SSE42 1
    #endif
#elif (HE_TB64_CPU_ARM_32 || HE_TB64_CPU_ARM_64) && (HE_COMPILER_MSVC || defined(__ARM_NEON__) || defined(__ARM_NEON))
    #if HE_TB64_CPU_ARM_64
        #undef HAVE_NEON64
        #define HAVE_NEON64 1
    #else
        #undef HAVE_NEON32
        #define HAVE_NEON32 1
    #endif
#endif
