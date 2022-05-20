// Copyright Chad Engler

#include "he/core/compiler.h"

#include "he/core/test.h"
#include "he/core/utils.h"

#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace he;

// ------------------------------------------------------------------------------------------------
extern "C" HE_DLL_EXPORT void _HeTestingDllExport()
{
    HE_EXPECT_EQ_STR(HE_FUNC_SIG, "void __cdecl _HeTestingDllExport(void)");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, Report)
{
    std::cout << "    HE_COMPILER_CLANG = " << HE_COMPILER_CLANG << std::endl;
    std::cout << "    HE_COMPILER_GCC = " << HE_COMPILER_GCC << std::endl;
    std::cout << "    HE_COMPILER_MSVC = " << HE_COMPILER_MSVC << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, FUNC_SIG)
{
    HE_EXPECT_GT(HE_LENGTH_OF(HE_FUNC_SIG), 0);
    std::cout << "    HE_FUNC_SIG = " << HE_FUNC_SIG << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, FORCE_INLINE)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, NO_INLINE)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, LIKELY)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    int val = std::rand();

    if (HE_LIKELY((val % 2) == 1))
    {
        std::cout << "    Likely odd" << std::endl;
    }
    else
    {
        std::cout << "    Unlikely even" << std::endl;
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, UNLIKELY)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    int val = std::rand();

    if (HE_UNLIKELY((val % 2) == 1))
    {
        std::cout << "    Unlikely odd" << std::endl;
    }
    else
    {
        std::cout << "    Likely even" << std::endl;
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, DLL_EXPORT)
{
    _HeTestingDllExport();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, UNREACHABLE)
{
    if (true)
        return;

    HE_UNREACHABLE();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, compiler, WARNINGS)
{
    HE_PUSH_WARNINGS();
    HE_DISABLE_MSVC_WARNING(4101); // unused var

    int unused;

    HE_POP_WARNINGS();
}
