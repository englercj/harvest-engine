// Copyright Chad Engler

#pragma once

#include "he/core/macros.h"

#define HE_COMPILER_CLANG                   0
#define HE_COMPILER_GCC                     0
#define HE_COMPILER_MSVC                    0

#if defined(__has_builtin)
    #define HE_HAS_BUILTIN(x) __has_builtin(x)
#else
    #define HE_HAS_BUILTIN(x) 0
#endif

#if defined(__clang__)
    #undef  HE_COMPILER_CLANG
    #define HE_COMPILER_CLANG               (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)

    #if HE_COMPILER_CLANG < 100000
        #error "Clang 10+ is required."
    #endif

    #define HE_ALIGNED(N)                   __attribute__((aligned(N)))
    #define HE_FUNC_SIG                     __PRETTY_FUNCTION__
    #define HE_FORCE_INLINE                 __attribute__((always_inline)) inline
    #define HE_NO_INLINE                    __attribute__((noinline))
    #define HE_LIKELY(x)                    __builtin_expect(!!(x), 1)
    #define HE_UNLIKELY(x)                  __builtin_expect(!!(x), 0)
    #define HE_DLL_EXPORT                   __attribute__((visibility("default")))
    #define HE_UNREACHABLE()                __builtin_unreachable()

    #define HE_PUSH_WARNINGS()              _Pragma("clang diagnostic push")
    #define HE_POP_WARNINGS()               _Pragma("clang diagnostic pop")
    #define HE_DISABLE_CLANG_WARNING(n)     _Pragma(HE_STRINGIFY(clang diagnostic ignored n))
    #define HE_DISABLE_GCC_WARNING(n)
    #define HE_DISABLE_GCC_CLANG_WARNING(n) HE_DISABLE_CLANG_WARNING(n)
    #define HE_DISABLE_MSVC_WARNING(n)
#elif defined(__GNUC__)
    #undef  HE_COMPILER_GCC
    #define HE_COMPILER_GCC                 (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

    #if HE_COMPILER_GCC < 80300
        #error "GCC 8.3.0+ is required."
    #endif

    #define HE_ALIGNED(N)                   __attribute__((aligned(N)))
    #define HE_FUNC_SIG                     __PRETTY_FUNCTION__
    #define HE_FORCE_INLINE                 __attribute__((always_inline)) inline
    #define HE_NO_INLINE                    __attribute__((noinline))
    #define HE_LIKELY(x)                    __builtin_expect(!!(x), 1)
    #define HE_UNLIKELY(x)                  __builtin_expect(!!(x), 0)
    #define HE_DLL_EXPORT                   __attribute__((visibility("default")))
    #define HE_UNREACHABLE()                __builtin_unreachable()

    #define HE_PUSH_WARNINGS()              _Pragma("GCC diagnostic push")
    #define HE_POP_WARNINGS()               _Pragma("GCC diagnostic pop")
    #define HE_DISABLE_CLANG_WARNING(n)
    #define HE_DISABLE_GCC_WARNING(n)       _Pragma(HE_STRINGIFY(GCC diagnostic ignored n))
    #define HE_DISABLE_GCC_CLANG_WARNING(n) HE_DISABLE_GCC_WARNING(n)
    #define HE_DISABLE_MSVC_WARNING(n)
#elif defined(_MSC_VER)
    #undef  HE_COMPILER_MSVC
    #define HE_COMPILER_MSVC                (_MSC_VER)

    // Visual Studio 2019 >= 16.9 required
    #if HE_COMPILER_MSVC < 1929
        #error "MSVC 14.29 (VS2019 v16.9) is required."
    #endif

    #define HE_ALIGNED(N)                   __declspec(align(N))
    #define HE_FUNC_SIG                     __FUNCSIG__
    #define HE_FORCE_INLINE                 __forceinline
    #define HE_NO_INLINE                    __declspec(noinline)
    #define HE_LIKELY(x)                    (x)
    #define HE_UNLIKELY(x)                  (x)
    #define HE_DLL_EXPORT                   __declspec(dllexport)
    #define HE_UNREACHABLE()                __assume(0)

    #define HE_PUSH_WARNINGS()              __pragma(warning(push))
    #define HE_POP_WARNINGS()               __pragma(warning(pop))
    #define HE_DISABLE_CLANG_WARNING(n)
    #define HE_DISABLE_GCC_WARNING(n)
    #define HE_DISABLE_GCC_CLANG_WARNING(n)
    #define HE_DISABLE_MSVC_WARNING(n)      __pragma(warning(disable: n))
#else
    #error "Unknown compiler"
#endif
