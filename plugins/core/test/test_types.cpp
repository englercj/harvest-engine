// Copyright Chad Engler

#include "he/core/types.h"

#include "he/core/cpu.h"
#include "he/core/type_traits.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, types, Test)
{
    static_assert(sizeof(bool) == 1);
    static_assert(sizeof(char) == 1);
    static_assert(sizeof(short) == 2);
    static_assert(sizeof(int) == 4);
    static_assert(sizeof(long long) == 8);

    static_assert(sizeof(int8_t) == 1);
    static_assert(sizeof(int16_t) == 2);
    static_assert(sizeof(int32_t) == 4);
    static_assert(sizeof(int64_t) == 8);

    static_assert(sizeof(uint8_t) == 1);
    static_assert(sizeof(uint16_t) == 2);
    static_assert(sizeof(uint32_t) == 4);
    static_assert(sizeof(uint64_t) == 8);

    constexpr uint32_t PtrSize = HE_CPU_64_BIT ? 8 : 4;
    static_assert(sizeof(size_t) == PtrSize);
    static_assert(sizeof(intptr_t) == PtrSize);
    static_assert(sizeof(uintptr_t) == PtrSize);

    static_assert(IsSame<decltype(DefaultInit), const DefaultInitTag>);
}
