// Copyright Chad Engler

#include "he/core/enum_ops.h"
#include "he/rhi/types.h"

namespace he
{
    template <>
    const char* AsString(rhi::ApiResult x)
    {
        switch (x)
        {
            case rhi::ApiResult::Success: return "Success";
            case rhi::ApiResult::Failure: return "Failure";
            case rhi::ApiResult::DeviceLost: return "DeviceLost";
            case rhi::ApiResult::OutOfMemory: return "OutOfMemory";
            case rhi::ApiResult::NotFound: return "NotFound";
        }

        return "<unknown>";
    }

    template <>
    const char* AsString(rhi::Format x)
    {
        switch (x)
        {
            case rhi::Format::Invalid: return "Invalid";
            case rhi::Format::R8Unorm: return "R8Unorm";
            case rhi::Format::R8Snorm: return "R8Snorm";
            case rhi::Format::R8Uint: return "R8Uint";
            case rhi::Format::R8Sint: return "R8Sint";
            case rhi::Format::A8Unorm: return "A8Unorm";
            case rhi::Format::R16Unorm: return "R16Unorm";
            case rhi::Format::R16Snorm: return "R16Snorm";
            case rhi::Format::R16Uint: return "R16Uint";
            case rhi::Format::R16Sint: return "R16Sint";
            case rhi::Format::R16Float: return "R16Float";
            case rhi::Format::RG8Unorm: return "RG8Unorm";
            case rhi::Format::RG8Snorm: return "RG8Snorm";
            case rhi::Format::RG8Uint: return "RG8Uint";
            case rhi::Format::RG8Sint: return "RG8Sint";
            case rhi::Format::BGRA4Unorm: return "BGRA4Unorm";
            case rhi::Format::B5G6R5Unorm: return "B5G6R5Unorm";
            case rhi::Format::R32Uint: return "R32Uint";
            case rhi::Format::R32Sint: return "R32Sint";
            case rhi::Format::R32Float: return "R32Float";
            case rhi::Format::RG16Unorm: return "RG16Unorm";
            case rhi::Format::RG16Snorm: return "RG16Snorm";
            case rhi::Format::RG16Uint: return "RG16Uint";
            case rhi::Format::RG16Sint: return "RG16Sint";
            case rhi::Format::RG16Float: return "RG16Float";
            case rhi::Format::RGBA8Unorm: return "RGBA8Unorm";
            case rhi::Format::RGBA8Unorm_sRGB: return "RGBA8Unorm_sRGB";
            case rhi::Format::RGBA8Snorm: return "RGBA8Snorm";
            case rhi::Format::RGBA8Uint: return "RGBA8Uint";
            case rhi::Format::RGBA8Sint: return "RGBA8Sint";
            case rhi::Format::BGRA8Unorm: return "BGRA8Unorm";
            case rhi::Format::BGRA8Unorm_sRGB: return "BGRA8Unorm_sRGB";
            case rhi::Format::BGRX8Unorm: return "BGRX8Unorm";
            case rhi::Format::BGRX8Unorm_sRGB: return "BGRX8Unorm_sRGB";
            case rhi::Format::RGB10A2Unorm: return "RGB10A2Unorm";
            case rhi::Format::RGB10A2Uint: return "RGB10A2Uint";
            case rhi::Format::RG11B10Float: return "RG11B10Float";
            case rhi::Format::RGB9E5SharedExp: return "RGB9E5SharedExp";
            case rhi::Format::RG32Uint: return "RG32Uint";
            case rhi::Format::RG32Sint: return "RG32Sint";
            case rhi::Format::RG32Float: return "RG32Float";
            case rhi::Format::RGBA16Unorm: return "RGBA16Unorm";
            case rhi::Format::RGBA16Snorm: return "RGBA16Snorm";
            case rhi::Format::RGBA16Uint: return "RGBA16Uint";
            case rhi::Format::RGBA16Sint: return "RGBA16Sint";
            case rhi::Format::RGBA16Float: return "RGBA16Float";
            case rhi::Format::RGB32Uint: return "RGB32Uint";
            case rhi::Format::RGB32Sint: return "RGB32Sint";
            case rhi::Format::RGB32Float: return "RGB32Float";
            case rhi::Format::RGBA32Uint: return "RGBA32Uint";
            case rhi::Format::RGBA32Sint: return "RGBA32Sint";
            case rhi::Format::RGBA32Float: return "RGBA32Float";
            case rhi::Format::BC1_RGBA: return "BC1_RGBA";
            case rhi::Format::BC1_RGBA_sRGB: return "BC1_RGBA_sRGB";
            case rhi::Format::BC2_RGBA: return "BC2_RGBA";
            case rhi::Format::BC2_RGBA_sRGB: return "BC2_RGBA_sRGB";
            case rhi::Format::BC3_RGBA: return "BC3_RGBA";
            case rhi::Format::BC3_RGBA_sRGB: return "BC3_RGBA_sRGB";
            case rhi::Format::BC4_RUnorm: return "BC4_RUnorm";
            case rhi::Format::BC4_RSnorm: return "BC4_RSnorm";
            case rhi::Format::BC5_RGUnorm: return "BC5_RGUnorm";
            case rhi::Format::BC5_RGSnorm: return "BC5_RGSnorm";
            case rhi::Format::BC6H_RGBFloat: return "BC6H_RGBFloat";
            case rhi::Format::BC6H_RGBUfloat: return "BC6H_RGBUfloat";
            case rhi::Format::BC7_RGBA: return "BC7_RGBA";
            case rhi::Format::BC7_RGBA_sRGB: return "BC7_RGBA_sRGB";
            case rhi::Format::Depth16Unorm: return "Depth16Unorm";
            case rhi::Format::Depth32Float: return "Depth32Float";
            case rhi::Format::Depth24Unorm_Stencil8: return "Depth24Unorm_Stencil8";
            case rhi::Format::Depth32Float_Stencil8: return "Depth32Float_Stencil8";
            case rhi::Format::_Count: return "_Count";
        }

        return "<unknown>";
    }
}
