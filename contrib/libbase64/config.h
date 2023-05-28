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

// Define all the instruction set options that could be available for each architecture.
// The runtime CPUID checks will pick the best instruction for the current CPU.
#if defined(_M_IX86) || defined(__i386__) || defined(_M_AMD64) || defined(__x86_64__)
    #undef BASE64_WITH_SSSE3
    #define BASE64_WITH_SSSE3 1

    #undef BASE64_WITH_SSE41
    #define BASE64_WITH_SSE41 1

    #undef BASE64_WITH_SSE42
    #define BASE64_WITH_SSE42 1

    #undef BASE64_WITH_AVX
    #define BASE64_WITH_AVX 1

    #undef BASE64_WITH_AVX2
    #define BASE64_WITH_AVX2 1
#elif defined(_M_ARM) || defined(__arm__)
    #undef HAVE_NEON32
    #define HAVE_NEON32 1
#elif defined(_M_ARM64) || defined(__aarch64__)
    #undef HAVE_NEON64
    #define HAVE_NEON64 1
#endif
