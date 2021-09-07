// Copyright Chad Engler

#pragma once

#include "he/core/config.h"

/// \def HE_RHI_ENABLE_NULL
/// Controls if the null RHI backend is enabled. By default it is disabled.
#if !defined(HE_RHI_ENABLE_NULL)
    #define HE_RHI_ENABLE_NULL          0
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

/// \def HE_RHI_ENABLE_VULKAN
/// Controls if the Vulkan RHI backend is enabled. By default it is enabled for Windows and Linux.
#if !defined(HE_RHI_ENABLE_VULKAN)
    #if defined(HE_PLATFORM_WINDOWS) || defined(HE_PLATFORM_LINUX)
        #define HE_RHI_ENABLE_VULKAN    1
    #else
        #define HE_RHI_ENABLE_VULKAN    0
    #endif
#endif

/// \def HE_RHI_ENABLE_WEBGPU
/// Controls if the WebGPU RHI backend is enabled. By default it is enabled for Emscripten.
#if !defined(HE_RHI_ENABLE_WEBGPU)
    #if defined(HE_PLATFORM_EMSCRIPTEN)
        #define HE_RHI_ENABLE_WEBGPU    1
    #else
        #define HE_RHI_ENABLE_WEBGPU    0
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
