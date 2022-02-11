// Copyright Chad Engler

#include "he/core/debug.h"

#include "he/core/test.h"

#include <iostream>
#include <type_traits>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, debug, File_Line)
{
    static_assert(std::is_convertible_v<decltype(HE_FILE), const char*>);
    static_assert(std::is_same_v<decltype(HE_LINE), uint32_t>);

    std::cout << "    HE_FILE = " << HE_FILE << std::endl;
    std::cout << "    HE_LINE = " << HE_LINE << std::endl;
}
