// Copyright Chad Engler

#include "he/core/compiler.h"

#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, Report)
{
    std::cout << "    HE_COMPILER_CLANG = " << HE_COMPILER_CLANG << std::endl;
    std::cout << "    HE_COMPILER_GCC = " << HE_COMPILER_GCC << std::endl;
    std::cout << "    HE_COMPILER_MSVC = " << HE_COMPILER_MSVC << std::endl;

    std::cout << "    HE_FUNC_SIG = " << HE_FUNC_SIG << std::endl;
}
