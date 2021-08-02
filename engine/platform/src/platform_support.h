// Copyright Chad Engler

#pragma once

// The platforms this module provides support for. This macro is just to make it easier to check
// if we should provide implementations or not.
#if defined(HE_PLATFORM_LINUX) || defined(HE_PLATFORM_EMSCRIPTEN) || defined(HE_PLATFORM_API_WIN32)
    #define HE_CAN_PROVIDE_PLATFORM 1
#else
    #define HE_CAN_PROVIDE_PLATFORM 0
#endif
