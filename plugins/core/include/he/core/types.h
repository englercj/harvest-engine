// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"

using int8_t = signed char;
using int16_t = short;
using int32_t = int;
using int64_t = long long;

using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;
using uint64_t = unsigned long long;

using nullptr_t = decltype(nullptr);

#if defined(__SIZE_TYPE__)
    using size_t = __SIZE_TYPE__;
#else
    using size_t = decltype(sizeof(0));
#endif

#if defined(__PTRDIFF_TYPE__)
    using ptrdiff_t = __PTRDIFF_TYPE__;
    using intptr_t = __PTRDIFF_TYPE__;
    using uintptr_t = unsigned __PTRDIFF_TYPE__;
#else
    using ptrdiff_t = decltype(reinterpret_cast<char*>(0) - reinterpret_cast<char*>(0));
    using intptr_t = ptrdiff_t;

    #if HE_HAS_BUILTIN(__make_unsigned)
        using uintptr_t = __make_unsigned(ptrdiff_t);
    #else
        using uintptr_t = sizeof(ptrdiff_t) == 8 ? unsigned long long
            : sizeof(ptrdiff_t) == 4 ? unsigned long
            : sizeof(ptrdiff_t) == 2 ? unsigned short
            : sizeof(ptrdiff_t) == 1 ? unsigned char;
    #endif
#endif

#if defined(__WINT_TYPE__)
    using wint_t = __WINT_TYPE__;
#else
    using wint_t = int;
#endif

namespace he
{
    struct DefaultInitTag {};
    constexpr DefaultInitTag DefaultInit{};

    typedef struct { long long ll; long double ld; } MaxAlign;
}
