// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"

/// Set to one (1) when the target is any ARM platform, and zero (0) otherwise.
#define HE_CPU_ARM                  0

/// Set to one (1) when the target is 32-bit ARM platforms, and zero (0) otherwise.
#define HE_CPU_ARM_32               0

/// Set to one (1) when the target is 64-bit ARM platforms, and zero (0) otherwise.
#define HE_CPU_ARM_64               0

/// Set to one (1) when the target is any WASM platform, and zero (0) otherwise.
#define HE_CPU_WASM                 0

/// Set to one (1) when the target is 32-bit WASM platforms, and zero (0) otherwise.
#define HE_CPU_WASM_32              0

/// Set to one (1) when the target is 64-bit WASM platforms, and zero (0) otherwise.
#define HE_CPU_WASM_64              0

/// Set to one (1) when the target is any x86 platform, and zero (0) otherwise.
#define HE_CPU_X86                  0

/// Set to one (1) when the target is 32-bit x86 platforms, and zero (0) otherwise.
#define HE_CPU_X86_32               0

/// Set to one (1) when the target is 64-bit x86 platforms, and zero (0) otherwise.
#define HE_CPU_X86_64               0

/// Set to one (1) when the target is any 64-bit platform, and zero (0) otherwise.
#define HE_CPU_64_BIT               0

#if defined(__BIG_ENDIAN__) || defined(__ARMEB__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #error "Big endian systems are not supported by the Harvest engine."
#endif

#if defined(_M_ARM) || defined(__arm__)
    #undef  HE_CPU_ARM_32
    #define HE_CPU_ARM_32           1
#elif defined(_M_ARM64) || defined(__aarch64__)
    #undef  HE_CPU_ARM_64
    #define HE_CPU_ARM_64           1
#elif defined(__wasm32)
    #undef  HE_CPU_WASM_32
    #define HE_CPU_WASM_32          1
#elif defined(__wasm64)
    #undef  HE_CPU_WASM_64
    #define HE_CPU_WASM_64          1
#elif defined(_M_IX86) || defined(__i386__)
    #undef  HE_CPU_X86_32
    #define HE_CPU_X86_32           1
#elif defined(_M_AMD64) || defined(__x86_64__)
    #undef  HE_CPU_X86_64
    #define HE_CPU_X86_64           1
#endif

#if HE_CPU_ARM_32 || HE_CPU_ARM_64
    #undef  HE_CPU_ARM
    #define HE_CPU_ARM              1
#elif HE_CPU_WASM_32 || HE_CPU_WASM_64
    #undef  HE_CPU_WASM
    #define HE_CPU_WASM             1
#elif HE_CPU_X86_32 || HE_CPU_X86_64
    #undef  HE_CPU_X86
    #define HE_CPU_X86              1
#else
    #error "Unsupported processor"
#endif

#if HE_CPU_ARM_64 || HE_CPU_WASM_64 || HE_CPU_X86_64
    #undef  HE_CPU_64_BIT
    #define HE_CPU_64_BIT           1
#endif

/// \def HE_SPIN_WAIT_PAUSE
/// Causes a cpu with dynamic execution to pause the next instruction for an
/// implementation-specific amount of time. This is useful in spin-wait loops.
#if HE_COMPILER_MSVC
    #if HE_CPU_ARM
        extern "C" void __yield(void);
        #pragma intrinsic(__yield)
        #define HE_SPIN_WAIT_PAUSE()    __yield()
    #elif HE_CPU_WASM
        #error "Wasm not supported via MSVC yet"
    #elif HE_CPU_X86
        extern "C" void _mm_pause(void);
        #pragma intrinsic(_mm_pause)
        #define HE_SPIN_WAIT_PAUSE()    _mm_pause()
    #endif
#else
    #if HE_CPU_ARM
        #define HE_SPIN_WAIT_PAUSE()    asm volatile("yield;" ::: "memory")
    #elif HE_CPU_WASM
        #define HE_SPIN_WAIT_PAUSE()    _mm_pause()
    #elif HE_CPU_X86
        #define HE_SPIN_WAIT_PAUSE()    asm volatile("pause;" ::: "memory")
    #endif
#endif
