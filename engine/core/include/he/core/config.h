// Copyright Chad Engler

#pragma once

#if defined(_DEBUG)
    #define HE_DEBUG                    1
    #define HE_RELEASE                  0
#else
    #define HE_DEBUG                    0
    #define HE_RELEASE                  1
#endif

#ifndef HE_INTERNAL_BUILD
    #define HE_INTERNAL_BUILD           HE_DEBUG
#endif

#ifndef HE_ASSERTIONS_ENABLED
    #define HE_ASSERTIONS_ENABLED       HE_INTERNAL_BUILD
#endif

#ifndef HE_ENABLE_MEMORY_TRACKING
    #define HE_ENABLE_MEMORY_TRACKING   HE_INTERNAL_BUILD
#endif
