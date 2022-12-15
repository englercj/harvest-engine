// Copyright Chad Engler

#include "he/core/asan.h"

#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, asan, Report)
{
    std::cout << "    HE_HAS_ASAN = " << HE_HAS_ASAN << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, asan, Poison)
{
    uint8_t mem[16]{};

    // poison the memory block
    HE_ASAN_POISON_MEMORY(mem, 16);

    // write/read in an unpoisoned region
    HE_ASAN_UNPOISON_MEMORY(mem + 8, 1);
    mem[8] = 1;
    HE_EXPECT_EQ(mem[8], 1);

    // unpoison everything
    HE_ASAN_UNPOISON_MEMORY(mem, 16);
}
