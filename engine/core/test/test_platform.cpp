// Copyright Chad Engler

#include "he/core/platform.h"

#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, platform, Report)
{
    std::cout << "HE_PLATFORM_EMSCRIPTEN = " << HE_PLATFORM_EMSCRIPTEN << std::endl;
    std::cout << "HE_PLATFORM_LINUX = " << HE_PLATFORM_LINUX << std::endl;
    std::cout << "HE_PLATFORM_WINDOWS = " << HE_PLATFORM_WINDOWS << std::endl;

    std::cout << "HE_API_WIN32 = " << HE_API_WIN32 << std::endl;
    std::cout << "HE_API_POSIX = " << HE_API_POSIX << std::endl;
    std::cout << "HE_API_SCE = " << HE_API_SCE << std::endl;
    std::cout << "HE_API_NN = " << HE_API_NN << std::endl;

    std::cout << "HE_API_DESKTOP = " << HE_API_DESKTOP << std::endl;
    std::cout << "HE_API_CONSOLE = " << HE_API_CONSOLE << std::endl;
    std::cout << "HE_API_WEB = " << HE_API_WEB << std::endl;
}
