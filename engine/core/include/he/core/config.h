// Copyright Chad Engler

#pragma once

#if !defined(HE_INTERNAL_BUILD)
    #if defined(HE_CFG_SHIPPING)
        #define HE_INTERNAL_BUILD       0
    #else
        #define HE_INTERNAL_BUILD       1
    #endif
#endif

#if !defined(HE_ENABLE_ASSERTIONS)
    #define HE_ENABLE_ASSERTIONS        HE_INTERNAL_BUILD
#endif

#if !defined(HE_ENABLE_MEMORY_TRACKING)
    #define HE_ENABLE_MEMORY_TRACKING   HE_INTERNAL_BUILD
#endif

#if !defined(HE_ENABLE_DYNAMIC_MODULES)
    #define HE_ENABLE_DYNAMIC_MODULES   HE_INTERNAL_BUILD
#endif

#if !defined(HE_ENABLE_SIMD)
    #define HE_ENABLE_SIMD              1
#endif
