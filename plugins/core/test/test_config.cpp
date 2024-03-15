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

#if defined(HE_CFG_DEVELOPMENT)
    std::cout << "    HE_CFG_DEVELOPMENT = " << HE_CFG_DEVELOPMENT << std::endl;
#else
    std::cout << "    HE_CFG_DEVELOPMENT not defined" << std::endl;
#endif

#if defined(HE_CFG_RELEASE)
    std::cout << "    HE_CFG_RELEASE = " << HE_CFG_RELEASE << std::endl;
#else
    std::cout << "    HE_CFG_RELEASE not defined" << std::endl;
#endif

#if defined(HE_PLATFORM_API_WASM)
    std::cout << "    HE_PLATFORM_API_WASM = " << HE_PLATFORM_API_WASM << std::endl;
#else
    std::cout << "    HE_PLATFORM_API_WASM not defined" << std::endl;
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
    std::cout << "    HE_ENABLE_DEFAULT_ALLOCATOR = " << HE_ENABLE_DEFAULT_ALLOCATOR << std::endl;
}
