// Copyright Chad Engler

#include "he/core/config.h"

#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, config, Report)
{
#if defined(HE_CFG_DEBUG)
    std::cout << "    HE_CFG_DEBUG = " << HE_CFG_DEBUG << std::endl;
#else
    std::cout << "    HE_CFG_DEBUG not defined" << std::endl;
#endif

#if defined(HE_CFG_RELEASE)
    std::cout << "    HE_CFG_RELEASE = " << HE_CFG_RELEASE << std::endl;
#else
    std::cout << "    HE_CFG_RELEASE not defined" << std::endl;
#endif

#if defined(HE_CFG_SHIPPING)
    std::cout << "    HE_CFG_SHIPPING = " << HE_CFG_SHIPPING << std::endl;
#else
    std::cout << "    HE_CFG_SHIPPING not defined" << std::endl;
#endif

#if defined(HE_PLATFORM_EMSCRIPTEN)
    std::cout << "    HE_PLATFORM_EMSCRIPTEN = " << HE_PLATFORM_EMSCRIPTEN << std::endl;
#else
    std::cout << "    HE_PLATFORM_EMSCRIPTEN not defined" << std::endl;
#endif

#if defined(HE_PLATFORM_LINUX)
    std::cout << "    HE_PLATFORM_LINUX = " << HE_PLATFORM_LINUX << std::endl;
#else
    std::cout << "    HE_PLATFORM_LINUX not defined" << std::endl;
#endif

#if defined(HE_PLATFORM_WINDOWS)
    std::cout << "    HE_PLATFORM_WINDOWS = " << HE_PLATFORM_WINDOWS << std::endl;
#else
    std::cout << "    HE_PLATFORM_WINDOWS not defined" << std::endl;
#endif

#if defined(HE_PLATFORM_API_POSIX)
    std::cout << "    HE_PLATFORM_API_POSIX = " << HE_PLATFORM_API_POSIX << std::endl;
#else
    std::cout << "    HE_PLATFORM_API_POSIX not defined" << std::endl;
#endif

#if defined(HE_PLATFORM_API_WIN32)
    std::cout << "    HE_PLATFORM_API_WIN32 = " << HE_PLATFORM_API_WIN32 << std::endl;
#else
    std::cout << "    HE_PLATFORM_API_WIN32 not defined" << std::endl;
#endif

    std::cout << "    HE_INTERNAL_BUILD = " << HE_INTERNAL_BUILD << std::endl;
    std::cout << "    HE_ENABLE_ASSERTIONS = " << HE_ENABLE_ASSERTIONS << std::endl;
    std::cout << "    HE_USER_DEFINED_DEFAULT_ALLOCATOR = " << HE_USER_DEFINED_DEFAULT_ALLOCATOR << std::endl;
}
