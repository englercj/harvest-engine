// Copyright Chad Engler

#pragma once

/// \def HE_INTERNAL_BUILD
/// Controls if this build is considered Internal. By default this is enabled for non-Shipping
/// configurations.
#if !defined(HE_INTERNAL_BUILD)
    #if defined(HE_CFG_SHIPPING)
        #define HE_INTERNAL_BUILD       0
    #else
        #define HE_INTERNAL_BUILD       1
    #endif
#endif

/// \def HE_ENABLE_ASSERTIONS
/// Controls if assertions are checked. By default these are enabled for Internal builds.
#if !defined(HE_ENABLE_ASSERTIONS)
    #define HE_ENABLE_ASSERTIONS        HE_INTERNAL_BUILD
#endif

/// \def HE_ENABLE_SIMD
/// Controls if SIMD instructions can be used. By default this is enabled.
#if !defined(HE_ENABLE_SIMD)
    #define HE_ENABLE_SIMD              1
#endif
