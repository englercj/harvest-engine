// Copyright Chad Engler

#pragma once

#include "he/rhi/types.h"

namespace he::rhi
{
    /// Returns true if a format is a depth format type.
    ///
    /// \param[in] x Then format to check.
    /// \return True if the format is a depth format.
    constexpr bool IsDepthFormat(Format x) noexcept
    {
        return x == Format::Depth16Unorm
            || x == Format::Depth32Float
            || x == Format::Depth24Unorm_Stencil8
            || x == Format::Depth32Float_Stencil8;
    }

    constexpr uint32_t PixelByteSize(Format x) noexcept
    {
        switch (x)
        {
            case Format::Invalid:
                return 0;

            // 8-bit formats
            case Format::R8Unorm:
            case Format::R8Snorm:
            case Format::R8Uint:
            case Format::R8Sint:
            case Format::A8Unorm:
                return 1;

            // 16-bit formats
            case Format::R16Unorm:
            case Format::R16Snorm:
            case Format::R16Uint:
            case Format::R16Sint:
            case Format::R16Float:
            case Format::RG8Unorm:
            case Format::RG8Snorm:
            case Format::RG8Uint:
            case Format::RG8Sint:
                return 2;

            // 16-bit packed formats
            case Format::BGRA4Unorm:
            case Format::B5G6R5Unorm:
                return 2;

            // 32-bit formats
            case Format::R32Uint:
            case Format::R32Sint:
            case Format::R32Float:
            case Format::RG16Unorm:
            case Format::RG16Snorm:
            case Format::RG16Uint:
            case Format::RG16Sint:
            case Format::RG16Float:
            case Format::RGBA8Unorm:
            case Format::RGBA8Unorm_sRGB:
            case Format::RGBA8Snorm:
            case Format::RGBA8Uint:
            case Format::RGBA8Sint:
            case Format::BGRA8Unorm:
            case Format::BGRA8Unorm_sRGB:
            case Format::BGRX8Unorm:
            case Format::BGRX8Unorm_sRGB:
                return 4;

            // 32-bit packed formats
            case Format::RGB10A2Unorm:
            case Format::RGB10A2Uint:
            case Format::RG11B10Float:
            case Format::RGB9E5SharedExp:
                return 4;

            // 64-bit formats
            case Format::RG32Uint:
            case Format::RG32Sint:
            case Format::RG32Float:
            case Format::RGBA16Unorm:
            case Format::RGBA16Snorm:
            case Format::RGBA16Uint:
            case Format::RGBA16Sint:
            case Format::RGBA16Float:
                return 8;

            // 96-bit formats
            case Format::RGB32Uint:
            case Format::RGB32Sint:
            case Format::RGB32Float:
                return 12;

            // 128-bit formats
            case Format::RGBA32Uint:
            case Format::RGBA32Sint:
            case Format::RGBA32Float:
                return 16;

            // Compressed BC formats
            case Format::BC1_RGBA:
            case Format::BC1_RGBA_sRGB:
            case Format::BC2_RGBA:
            case Format::BC2_RGBA_sRGB:
            case Format::BC3_RGBA:
            case Format::BC3_RGBA_sRGB:
            case Format::BC4_RUnorm:
            case Format::BC4_RSnorm:
            case Format::BC5_RGUnorm:
            case Format::BC5_RGSnorm:
            case Format::BC6H_RGBFloat:
            case Format::BC6H_RGBUfloat:
            case Format::BC7_RGBA:
            case Format::BC7_RGBA_sRGB:
                return 0;

            // Depth/stencil formats
            case Format::Depth16Unorm: return 2;
            case Format::Depth32Float: return 4;
            case Format::Depth24Unorm_Stencil8: return 4;
            case Format::Depth32Float_Stencil8: return 8;

            case Format::_Count:
                return 0;
        }

        return 0;
    }
}
