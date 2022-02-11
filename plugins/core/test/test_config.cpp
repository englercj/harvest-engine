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

    std::cout << "    HE_INTERNAL_BUILD = " << HE_INTERNAL_BUILD << std::endl;
    std::cout << "    HE_ENABLE_ASSERTIONS = " << HE_ENABLE_ASSERTIONS << std::endl;
    std::cout << "    HE_ENABLE_SIMD = " << HE_ENABLE_SIMD << std::endl;
    std::cout << "    HE_ENABLE_CUSTOM_ALLOCATORS = " << HE_ENABLE_CUSTOM_ALLOCATORS << std::endl;
}
