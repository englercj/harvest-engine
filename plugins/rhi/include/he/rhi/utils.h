// Copyright Chad Engler

#pragma once

#include "he/rhi/types.h"

namespace he::rhi
{
    /// Gets the default "preferred" rendering API for the current platform.
    ///
    /// \return Returns the preferred API type.
    constexpr ApiBackend GetDefaultApiBackend()
    {
    #if HE_RHI_ENABLE_D3D12
        return ApiBackend::D3D12;
    #elif HE_RHI_ENABLE_VULKAN
        return ApiBackend::Vulkan;
    #elif HE_RHI_ENABLE_WEBGPU
        return ApiBackend::WebGPU;
    #elif HE_RHI_ENABLE_NOOP
        return ApiBackend::Null;
    #else
        #error "No default render api available."
    #endif
    }

    /// Returns true if a format is a depth format type.
    ///
    /// \param[in] x Then format to check.
    /// \return True if the format is a depth format.
    constexpr bool IsDepthFormat(Format x)
    {
        return x == Format::Depth16Unorm
            || x == Format::Depth32Float
            || x == Format::Depth24Unorm_Stencil8
            || x == Format::Depth32Float_Stencil8;
    }
}
