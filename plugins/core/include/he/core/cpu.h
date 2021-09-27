// Copyright Chad Engler

#pragma once

#define HE_CPU_ARM                  0
#define HE_CPU_WASM                 0
#define HE_CPU_X86                  0

#define HE_CPU_ARM_32               0
#define HE_CPU_ARM_64               0

#define HE_CPU_WASM_32              0

#define HE_CPU_X86_32               0
#define HE_CPU_X86_64               0

#if defined(_M_ARM) || defined(__arm__)
    #undef  HE_CPU_ARM_32
    #define HE_CPU_ARM_32           1
#elif defined(_M_ARM64) || defined(__aarch64__)
    #undef  HE_CPU_ARM_64
    #define HE_CPU_ARM_64           1
#elif defined(__EMSCRIPTEN__)
    #undef  HE_CPU_WASM_32
    #define HE_CPU_WASM_32          1
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
#elif HE_CPU_WASM_32
    #undef  HE_CPU_WASM
    #define HE_CPU_WASM             1
#elif HE_CPU_X86_32 || HE_CPU_X86_64
    #undef  HE_CPU_X86
    #define HE_CPU_X86              1
#else
    #error "Unsupported processor"
#endif

#if HE_CPU_X86_64 || HE_CPU_ARM_64
    #define HE_CPU_64_BIT           1
#else
    #define HE_CPU_64_BIT           0
#endif

#if defined(__BIG_ENDIAN__) || defined(__ARMEB__)
    #error "Big endian systems are not supported."
#endif
