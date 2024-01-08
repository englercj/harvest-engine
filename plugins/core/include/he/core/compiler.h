// Copyright Chad Engler

#pragma once

#include "he/core/macros.h"

/// When the compiler is Clang this macro will hold the Clang version, otherwise is zero.
#define HE_COMPILER_CLANG                   0

/// When the compiler is GCC this macro will hold the GCC version, otherwise is zero.
#define HE_COMPILER_GCC                     0

/// When the compiler is MSVC this macro will hold the MSVC version, otherwise is zero.
#define HE_COMPILER_MSVC                    0

/// \def HE_HAS_BUILTIN
/// Checks if a builtin exists and evaluates to one if it does, otherwise zero
///
/// \param[in] x The builtin to check for.
/// \return Evaluates to 1 or 0 depending on if the builtin is available.
#if defined(__has_builtin)
    #define HE_HAS_BUILTIN(x) __has_builtin(x)
#else
    #define HE_HAS_BUILTIN(x) 0
#endif

/// \def HE_HAS_FEATURE
/// Checks if a feature is supported by the compiler.
///
/// \param[in] x The feature to check for.
/// \return Evaluates to 1 or 0 depending on if the feature in available.
#if defined(__has_feature)
    #define HE_HAS_FEATURE(x) __has_feature(x)
#else
    #define HE_HAS_FEATURE(x) 0
#endif

#if defined(__clang__)
    #undef  HE_COMPILER_CLANG
    #define HE_COMPILER_CLANG               (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)

    #if HE_COMPILER_CLANG < 110000
        #error "Clang 11+ is required."
    #endif

    #define HE_FUNC_SIG                     __PRETTY_FUNCTION__
    #define HE_FORCE_INLINE                 __attribute__((always_inline)) inline
    #define HE_NO_INLINE                    __attribute__((noinline))
    #define HE_EXPORT                       __attribute__((visibility("default")))
    #define HE_LIKELY(x)                    static_cast<bool>(__builtin_expect(!!(x), 1))
    #define HE_UNLIKELY(x)                  static_cast<bool>(__builtin_expect(!!(x), 0))
    #define HE_UNREACHABLE()                __builtin_unreachable()
    #define HE_PREFETCH(x)                  __builtin_prefetch(static_cast<const char*>(x))
    #define HE_SIZEOF_LONG                  8

    #define HE_PUSH_WARNINGS()              _Pragma("clang diagnostic push")
    #define HE_POP_WARNINGS()               _Pragma("clang diagnostic pop")
    #define HE_DISABLE_CLANG_WARNING(n)     _Pragma(HE_STRINGIFY(clang diagnostic ignored n))
    #define HE_DISABLE_GCC_WARNING(n)
    #define HE_DISABLE_GCC_CLANG_WARNING(n) HE_DISABLE_CLANG_WARNING(n)
    #define HE_DISABLE_MSVC_WARNING(n)

    #define HE_DISABLE_OPTIMIZE_START()     _Pragma("clang optimize off")
    #define HE_DISABLE_OPTIMIZE_END()       _Pragma("clang optimize on")
#elif defined(__GNUC__)
    #undef  HE_COMPILER_GCC
    #define HE_COMPILER_GCC                 (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

    #if HE_COMPILER_GCC < 110200
        #error "GCC 11.2.0+ is required."
    #endif

    #define HE_FUNC_SIG                     __PRETTY_FUNCTION__
    #define HE_FORCE_INLINE                 __attribute__((always_inline)) inline
    #define HE_NO_INLINE                    __attribute__((noinline))
    #define HE_EXPORT                       __attribute__((visibility("default")))
    #define HE_LIKELY(x)                    static_cast<bool>(__builtin_expect(!!(x), 1))
    #define HE_UNLIKELY(x)                  static_cast<bool>(__builtin_expect(!!(x), 0))
    #define HE_UNREACHABLE()                __builtin_unreachable()
    #define HE_PREFETCH(x)                  __builtin_prefetch(static_cast<const char*>(x))
    #define HE_SIZEOF_LONG                  8

    #define HE_PUSH_WARNINGS()              _Pragma("GCC diagnostic push")
    #define HE_POP_WARNINGS()               _Pragma("GCC diagnostic pop")
    #define HE_DISABLE_CLANG_WARNING(n)
    #define HE_DISABLE_GCC_WARNING(n)       _Pragma(HE_STRINGIFY(GCC diagnostic ignored n))
    #define HE_DISABLE_GCC_CLANG_WARNING(n) HE_DISABLE_GCC_WARNING(n)
    #define HE_DISABLE_MSVC_WARNING(n)

    #define HE_DISABLE_OPTIMIZE_START()     (_Pragma("GCC push_options"), _Pragma("GCC optimize (O0)"))
    #define HE_DISABLE_OPTIMIZE_END()       _Pragma("GCC pop_options")
#elif defined(_MSC_VER)
    #undef  HE_COMPILER_MSVC
    #define HE_COMPILER_MSVC                (_MSC_VER)

    // Visual Studio 2019 >= 16.10 required
    #if HE_COMPILER_MSVC < 1929
        #error "MSVC 14.29 (VS2019 v16.10) is required."
    #endif

    #define HE_FUNC_SIG                     __FUNCSIG__
    #define HE_FORCE_INLINE                 __forceinline
    #define HE_NO_INLINE                    __declspec(noinline)
    #define HE_EXPORT                       __declspec(dllexport)
    #define HE_LIKELY(x)                    (!!(x))
    #define HE_UNLIKELY(x)                  (!!(x))
    #define HE_UNREACHABLE()                __assume(0)
    #define HE_PREFETCH(x)                  _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0)
    #define HE_SIZEOF_LONG                  4

    #define HE_PUSH_WARNINGS()              __pragma(warning(push))
    #define HE_POP_WARNINGS()               __pragma(warning(pop))
    #define HE_DISABLE_CLANG_WARNING(n)
    #define HE_DISABLE_GCC_WARNING(n)
    #define HE_DISABLE_GCC_CLANG_WARNING(n)
    #define HE_DISABLE_MSVC_WARNING(n)      __pragma(warning(disable: n))

    #define HE_DISABLE_OPTIMIZE_START()     __pragma(optimize("", off))
    #define HE_DISABLE_OPTIMIZE_END()       __pragma(optimize("", on))
#else
    #error "Unknown compiler"
#endif

static_assert(HE_SIZEOF_LONG == sizeof(long));

/// \def HE_FUNC_SIG
/// The typed and fully qualified function signature

/// \def HE_FORCE_INLINE
/// Attribute for a function that tells the compiler to always inline it.

/// \def HE_NO_INLINE
/// Attribute for a function that tells the compiler to never inline it.

/// \def HE_EXPORT
/// Marks as symbol as exported so it is available in the symbols table.

/// \def HE_LIKELY
/// Marks a condition as likely to occur.
/// \note Prefer the C++20 [[likely]] attribute whenever possible.

/// \def HE_UNLIKELY
/// Marks a condition as unlikely to occur.
/// \note Prefer the C++20 [[unlikely]] attribute whenever possible.

/// \def HE_UNREACHABLE
/// Informs the compiler that this line of code cannot be reached.

/// \def HE_PREFETCH
/// Prefetches data from the specified address on one memory cache line.

/// \def HE_SIZEOF_LONG
/// Defined as the byte-size of `long` for this compiler.

/// \def HE_PUSH_WARNINGS
/// Stores the current warning state for all warnings.

/// \def HE_POP_WARNINGS
/// Restores the last warning state on the stack for all warnings.

/// \def HE_DISABLE_CLANG_WARNING
/// Disables a warning only when the compiler is clang.
///
/// \param[in] n The quoted string warning flag to disable. E.g.: "-Wunused-function"

/// \def HE_DISABLE_GCC_WARNING
/// Disables a warning only when the compiler is gcc.
///
/// \param[in] n The quoted string warning flag to disable. E.g.: "-Wunused-function"

/// \def HE_DISABLE_GCC_CLANG_WARNING
/// Disables a warning only when the compiler is gcc or clang.
///
/// \param[in] n The quoted string warning flag to disable. E.g.: "-Wunused-function"

/// \def HE_DISABLE_MSVC_WARNING
/// Disables a warning only when the compiler is msvc.
///
/// \param[in] n The diagnostic number of the warning to disable. E.g.: 4127

/// \def HE_DISABLE_OPTIMIZE_START
/// Starts a block in which code will not be optimized, despite compiler configuration.

/// \def HE_DISABLE_OPTIMIZE_END
/// Ends a block in which code will not be optimized, despite compiler configuration.
