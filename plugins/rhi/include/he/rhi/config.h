// Copyright Chad Engler

#pragma once

#include "he/core/config.h"

/// \def HE_RHI_ENABLE_NULL
/// Controls if the null RHI backend is enabled. By default it is disabled.
#if !defined(HE_RHI_ENABLE_NULL)
    #define HE_RHI_ENABLE_NULL          HE_INTERNAL_BUILD
#endif

/// \def HE_RHI_ENABLE_D3D12
/// Controls if the D3D12 RHI backend is enabled. By default it is enabled for Windows.
#if !defined(HE_RHI_ENABLE_D3D12)
    #if defined(HE_PLATFORM_WINDOWS)
        #define HE_RHI_ENABLE_D3D12     1
    #else
        #define HE_RHI_ENABLE_D3D12     0
    #endif
#endif

/// \def HE_RHI_ENABLE_NAMES
/// Controls if resource debug names are supported. By default it is enabled for Internal builds.
#if !defined(HE_RHI_ENABLE_NAMES)
    #define HE_RHI_ENABLE_NAMES         HE_INTERNAL_BUILD
#endif

/// \def HE_RHI_ENABLE_PIX
/// Controls if PIX profiling is enabled. By default it is enabled for Internal builds.
#if !defined(HE_RHI_ENABLE_PIX)
    #define HE_RHI_ENABLE_PIX           HE_INTERNAL_BUILD
#endif
