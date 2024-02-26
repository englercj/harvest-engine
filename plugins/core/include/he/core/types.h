// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"

using int8_t = signed char;
using int16_t = short;
using int32_t = int;

using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;

using nullptr_t = decltype(nullptr);

// 64-bit types
#if defined(__INT64_TYPE__)
    using int64_t = __INT64_TYPE__;
    using uint64_t = unsigned __INT64_TYPE__;
#else
    using int64_t = long long;
    using uint64_t = unsigned long long;
#endif

// 128-bit types
#if defined(__INT128_TYPE__)
    #define HE_HAS_INT128 1
    using int128_t = __INT128_TYPE__;
    using uint128_t = unsigned __INT128_TYPE__;
#elif defined(__SIZEOF_INT128__)
    #define HE_HAS_INT128 1
    using int128_t = __int128;
    using uint128_t = unsigned __int128;
#else
    #define HE_HAS_INT128 0
#endif

// size type
#if defined(__SIZE_TYPE__)
    using size_t = __SIZE_TYPE__;
#elif HE_COMPILER_MSVC
    #if HE_CPU_64_BIT
        using size_t = unsigned __int64;
    #else
        using size_t = unsigned int;
    #endif
#else
    using size_t = decltype(sizeof(0));
#endif

// ptr diff types
#if defined(__PTRDIFF_TYPE__)
    using ptrdiff_t = __PTRDIFF_TYPE__;
    using intptr_t = __PTRDIFF_TYPE__;
    using uintptr_t = unsigned __PTRDIFF_TYPE__;
#elif 0&& HE_COMPILER_MSVC
    #if HE_CPU_64_BIT
        using ptrdiff_t = __int64;
        using intptr_t = __int64;
        using uintptr_t = unsigned __int64;
    #else
        using ptrdiff_t = int;
        using intptr_t = int;
        using uintptr_t = unsigned int;
    #endif
#else
    using ptrdiff_t = decltype(reinterpret_cast<char*>(0) - reinterpret_cast<char*>(0));
    #if HE_CPU_64_BIT
        using intptr_t = long long;
        using uintptr_t = unsigned long long;
    #else
        using intptr_t = int;
        using uintptr_t = unsigned int;
    #endif
#endif

namespace he
{
    struct DefaultInitTag {};
    constexpr DefaultInitTag DefaultInit{};

    typedef struct { long long ll; long double ld; } MaxAlign;
}
