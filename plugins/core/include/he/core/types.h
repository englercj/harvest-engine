// Copyright Chad Engler

#pragma once

#include "he/core/cpu.h"

namespace he
{
    using int8_t = signed char;
    using int16_t = short;
    using int32_t = int;
    using int64_t = long long;

    using uint8_t = unsigned char;
    using uint16_t = unsigned short;
    using uint32_t = unsigned int;
    using uint64_t = unsigned long long;

#if HE_CPU_64_BIT
    using size_t = unsigned long long;
    using ptrdiff_t = long long;
    using intptr_t = long long;
    using uintptr_t = unsigned long long;
#else
    using size_t = unsigned int;
    using ptrdiff_t = int;
    using intptr_t = int;
    using uintptr_t = unsigned int;
#endif

    using max_align_t = double;

    using nullptr_t = decltype(nullptr);

    struct DefaultInitTag {};
    constexpr DefaultInitTag DefaultInit{};
}
