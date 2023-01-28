// Copyright Chad Engler

#include "he/core/debug.h"

#include "he/core/type_traits.h"
#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, debug, File_Line)
{
    static_assert(IsConvertible<decltype(HE_FILE), const char*>);
    static_assert(IsSame<decltype(HE_LINE), uint32_t>);

    std::cout << "    HE_FILE = " << HE_FILE << std::endl;
    std::cout << "    HE_LINE = " << HE_LINE << std::endl;
}
