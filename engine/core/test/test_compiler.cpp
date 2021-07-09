// Copyright Chad Engler

#include "he/core/compiler.h"

#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, Report)
{
    std::cout << "Compiler: ";
#if HE_COMPILER_CLANG
    std::cout << "Clang " << HE_COMPILER_CLANG;
#elif HE_COMPILER_GCC
    std::cout << "GCC " << HE_COMPILER_GCC;
#elif HE_COMPILER_MSVC
    std::cout << "MSVC " << HE_COMPILER_MSVC;
#endif
    std::cout << std::endl;

    std::cout << "HE_FUNC_SIG = " << HE_FUNC_SIG << std::endl;
}
