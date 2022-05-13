// Copyright Chad Engler

#include "d3d12_formats.h"

#include "he/core/assert.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    template <typename Map, typename Enum, size_t N>
    constexpr auto _ToDxType(const Map (&map)[N], Enum v) -> decltype(map[0])
    {
        static_assert(HE_LENGTH_OF(map) == static_cast<uint32_t>(Enum::_Count), "Length of D3D12 mapping does not match enum in function " HE_FUNC_SIG);
        HE_ASSERT(v < Enum::_Count);
        return map[static_cast<uint32_t>(v)];
    }

    // --------------------------------------------------------------------------------------------
    constexpr D3D12_DESCRIPTOR_RANGE_TYPE D3D12DescriptorRangeTypeMap[]
    {
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // StructuredBuffer
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // TypedBuffer
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // Texture
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV,        // RWStructuredBuffer
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV,        // RWTypedBuffer
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV,        // RWTexture
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // ConstantBuffer
        D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,    // Sampler
    };
    D3D12_DESCRIPTOR_RANGE_TYPE ToDxRangeType(DescriptorRangeType x) { return _ToDxType(D3D12DescriptorRangeTypeMap, x); }

    constexpr D3D12_RESOURCE_STATES D3D12BufferResourceStateMap[]
    {
        D3D12_RESOURCE_STATE_COMMON,                        // Common

        // Read
        D3D12_RESOURCE_STATE_INDEX_BUFFER,                  // Indices
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,    // Vertices
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,    // Constants
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,         // PixelShaderRead
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,   // AnyShaderRead
        D3D12_RESOURCE_STATE_COPY_SOURCE,                   // CopySrc

        // Write
        D3D12_RESOURCE_STATE_COMMON,                        // CpuWrite
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,              // ShaderReadWrite
        D3D12_RESOURCE_STATE_COPY_DEST,                     // CopyDst
    };
    D3D12_RESOURCE_STATES ToDxBufferResourceState(BufferState x) { return _ToDxType(D3D12BufferResourceStateMap, x); }

    constexpr D3D12_RESOURCE_STATES D3D12TextureResourceStateMap[]
    {
        D3D12_RESOURCE_STATE_COMMON, // Common

        // Read
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,         // PixelShaderRead
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        | D3D12_RESOURCE_STATE_DEPTH_READ,                  // PixelShaderDepthRead
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,     // NonPixelShaderRead
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        | D3D12_RESOURCE_STATE_DEPTH_READ,                  // NonPixelShaderDepthRead
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,   // AnyShaderRead
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        | D3D12_RESOURCE_STATE_DEPTH_READ,                  // AnyShaderDepthRead
        D3D12_RESOURCE_STATE_DEPTH_READ,                    // DepthRead
        D3D12_RESOURCE_STATE_COPY_SOURCE,                   // CopySrc
        D3D12_RESOURCE_STATE_PRESENT,                       // Present
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,                // ResolveSource

        // Write
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,              // ShaderReadWrite
        D3D12_RESOURCE_STATE_DEPTH_WRITE,                   // DepthWrite
        D3D12_RESOURCE_STATE_RENDER_TARGET,                 // Render
        D3D12_RESOURCE_STATE_COPY_DEST,                     // CopyDst
        D3D12_RESOURCE_STATE_RESOLVE_DEST,                  // ResolveDst
    };
    D3D12_RESOURCE_STATES ToDxTextureResourceState(TextureState x) { return _ToDxType(D3D12TextureResourceStateMap, x); }

    constexpr D3D12_HEAP_TYPE D3D12HeapTypeMap[]
    {
        D3D12_HEAP_TYPE_DEFAULT,    // Default
        D3D12_HEAP_TYPE_UPLOAD,     // Upload
        D3D12_HEAP_TYPE_READBACK,   // Readback
    };
    D3D12_HEAP_TYPE ToDxHeapType(HeapType x) { return _ToDxType(D3D12HeapTypeMap, x); }

    constexpr D3D12_COMMAND_LIST_TYPE D3D12CommandListTypeMap[]
    {
        D3D12_COMMAND_LIST_TYPE_COPY,       // Copy
        D3D12_COMMAND_LIST_TYPE_COMPUTE,    // Compute
        D3D12_COMMAND_LIST_TYPE_DIRECT,     // Graphics
    };
    D3D12_COMMAND_LIST_TYPE ToDxCmdListType(CmdListType x) { return _ToDxType(D3D12CommandListTypeMap, x); }

    constexpr D3D12_RESOURCE_DIMENSION D3D12TextureDimensionMap[]
    {
        D3D12_RESOURCE_DIMENSION_UNKNOWN,       // Unknown
        D3D12_RESOURCE_DIMENSION_TEXTURE1D,     // _1D
        D3D12_RESOURCE_DIMENSION_TEXTURE1D,     // _1DArray
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,     // _2D
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,     // _2DArray
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,     // _2DMS
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,     // _2DMSArray
        D3D12_RESOURCE_DIMENSION_TEXTURE3D,     // _3D
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,     // Cube
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,     // CubeArray
    };
    D3D12_RESOURCE_DIMENSION ToDxTextureDimension(TextureType x) { return _ToDxType(D3D12TextureDimensionMap, x); }

    constexpr D3D12_SRV_DIMENSION D3D12TextureSrvDimensionMap[]
    {
        D3D12_SRV_DIMENSION_UNKNOWN,            // Unknown
        D3D12_SRV_DIMENSION_TEXTURE1D,          // _1D
        D3D12_SRV_DIMENSION_TEXTURE1DARRAY,     // _1DArray
        D3D12_SRV_DIMENSION_TEXTURE2D,          // _2D
        D3D12_SRV_DIMENSION_TEXTURE2DARRAY,     // _2DArray
        D3D12_SRV_DIMENSION_TEXTURE2DMS,        // _2DMS
        D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY,   // _2DMSArray
        D3D12_SRV_DIMENSION_TEXTURE3D,          // _3D
        D3D12_SRV_DIMENSION_TEXTURECUBE,        // Cube
        D3D12_SRV_DIMENSION_TEXTURECUBEARRAY,   // CubeArray
    };
    D3D12_SRV_DIMENSION ToDxSrvDimension(TextureType x) { return _ToDxType(D3D12TextureSrvDimensionMap, x); }

    constexpr D3D12_UAV_DIMENSION D3D12TextureUavDimensionMap[]
    {
        D3D12_UAV_DIMENSION_UNKNOWN,            // Unknown
        D3D12_UAV_DIMENSION_TEXTURE1D,          // _1D
        D3D12_UAV_DIMENSION_TEXTURE1DARRAY,     // _1DArray
        D3D12_UAV_DIMENSION_TEXTURE2D,          // _2D
        D3D12_UAV_DIMENSION_TEXTURE2DARRAY,     // _2DArray
        D3D12_UAV_DIMENSION_UNKNOWN,            // _2DMS
        D3D12_UAV_DIMENSION_UNKNOWN,            // _2DMSArray
        D3D12_UAV_DIMENSION_TEXTURE3D,          // _3D
        D3D12_UAV_DIMENSION_TEXTURE2DARRAY,     // Cube
        D3D12_UAV_DIMENSION_TEXTURE2DARRAY,     // CubeArray
    };
    D3D12_UAV_DIMENSION ToDxUavDimension(TextureType x) { return _ToDxType(D3D12TextureUavDimensionMap, x); }

    constexpr D3D12_DSV_DIMENSION D3D12TextureDsvDimensionMap[]
    {
        D3D12_DSV_DIMENSION_UNKNOWN,            // Unknown
        D3D12_DSV_DIMENSION_TEXTURE1D,          // _1D
        D3D12_DSV_DIMENSION_TEXTURE1DARRAY,     // _1DArray
        D3D12_DSV_DIMENSION_TEXTURE2D,          // _2D
        D3D12_DSV_DIMENSION_TEXTURE2DARRAY,     // _2DArray
        D3D12_DSV_DIMENSION_TEXTURE2DMS,        // _2DMS
        D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY,   // _2DMSArray
        D3D12_DSV_DIMENSION_UNKNOWN,            // _3D
        D3D12_DSV_DIMENSION_TEXTURE2DARRAY,     // Cube
        D3D12_DSV_DIMENSION_TEXTURE2DARRAY,     // CubeArray
    };
    D3D12_DSV_DIMENSION ToDxDsvDimension(TextureType x) { return _ToDxType(D3D12TextureDsvDimensionMap, x); }

    constexpr D3D12_RTV_DIMENSION D3D12TextureRtvDimensionMap[]
    {
        D3D12_RTV_DIMENSION_UNKNOWN,            // Unknown
        D3D12_RTV_DIMENSION_TEXTURE1D,          // _1D
        D3D12_RTV_DIMENSION_TEXTURE1DARRAY,     // _1DArray
        D3D12_RTV_DIMENSION_TEXTURE2D,          // _2D
        D3D12_RTV_DIMENSION_TEXTURE2DARRAY,     // _2DArray
        D3D12_RTV_DIMENSION_TEXTURE2DMS,        // _2DMS
        D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY,   // _2DMSArray
        D3D12_RTV_DIMENSION_TEXTURE3D,          // _3D
        D3D12_RTV_DIMENSION_TEXTURE2DARRAY,     // Cube
        D3D12_RTV_DIMENSION_TEXTURE2DARRAY,     // CubeArray
    };
    D3D12_RTV_DIMENSION ToDxRtvDimension(TextureType x) { return _ToDxType(D3D12TextureRtvDimensionMap, x); }

    constexpr D3D12_SHADER_VISIBILITY D3D12ShaderVisibilityMap[]
    {
        D3D12_SHADER_VISIBILITY_ALL,        // All
        D3D12_SHADER_VISIBILITY_VERTEX,     // Vertex
        D3D12_SHADER_VISIBILITY_HULL,       // Hull
        D3D12_SHADER_VISIBILITY_DOMAIN,     // Domain
        D3D12_SHADER_VISIBILITY_GEOMETRY,   // Geometry
        D3D12_SHADER_VISIBILITY_PIXEL,      // Pixel
        D3D12_SHADER_VISIBILITY_ALL,        // Compute
    };
    D3D12_SHADER_VISIBILITY ToDxShaderVisibility(ShaderStage x) { return _ToDxType(D3D12ShaderVisibilityMap, x); }

    constexpr D3D12_PRIMITIVE_TOPOLOGY D3D12PrimitiveTopologyMap[]
    {
        D3D_PRIMITIVE_TOPOLOGY_POINTLIST,       // PointList
        D3D_PRIMITIVE_TOPOLOGY_LINELIST,        // LineList
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,       // LineStrip
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,    // TriList
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,   // TriStrip
    };
    D3D12_PRIMITIVE_TOPOLOGY ToDxPrimitiveTopology(PrimitiveType x) { return _ToDxType(D3D12PrimitiveTopologyMap, x); }

    constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12PrimitiveTopologyTypeMap[]
    {
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,        // PointList
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,         // LineList
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,         // LineStrip
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,     // TriList
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,     // TriStrip
    };
    D3D12_PRIMITIVE_TOPOLOGY_TYPE ToDxPrimitiveTopologyType(PrimitiveType x) { return _ToDxType(D3D12PrimitiveTopologyTypeMap, x); }

    constexpr D3D12_BLEND D3D12BlendFactorMap[]
    {
        D3D12_BLEND_ZERO,               // Zero
        D3D12_BLEND_ONE,                // One
        D3D12_BLEND_SRC_ALPHA,          // SrcAlpha
        D3D12_BLEND_INV_SRC_ALPHA,      // InvSrcAlpha
        D3D12_BLEND_SRC_COLOR,          // SrcColor
        D3D12_BLEND_INV_SRC_COLOR,      // InvSrcColor
        D3D12_BLEND_DEST_ALPHA,         // DstAlpha
        D3D12_BLEND_INV_DEST_ALPHA,     // InvDstAlpha
        D3D12_BLEND_DEST_COLOR,         // DstColor
        D3D12_BLEND_INV_DEST_COLOR,     // InvDstColor
        D3D12_BLEND_SRC_ALPHA_SAT,      // SrcAlphaSat
        D3D12_BLEND_BLEND_FACTOR,       // BlendFactor
        D3D12_BLEND_INV_BLEND_FACTOR,   // InvBlendFactor
        D3D12_BLEND_SRC1_ALPHA,         // Src1Alpha
        D3D12_BLEND_INV_SRC1_ALPHA,     // InvSrc1Alpha
        D3D12_BLEND_SRC1_COLOR,         // Src1Color
        D3D12_BLEND_INV_SRC1_COLOR,     // InvSrc1Color
    };
    D3D12_BLEND ToDxBlend(BlendFactor x) { return _ToDxType(D3D12BlendFactorMap, x); }

    constexpr D3D12_BLEND_OP D3D12BlendOpMap[]
    {
        D3D12_BLEND_OP_ADD,             // Add
        D3D12_BLEND_OP_SUBTRACT,        // Subtract
        D3D12_BLEND_OP_REV_SUBTRACT,    // RevSubtract
        D3D12_BLEND_OP_MIN,             // Min
        D3D12_BLEND_OP_MAX,             // Max
    };
    D3D12_BLEND_OP ToDxBlendOp(BlendOp x) { return _ToDxType(D3D12BlendOpMap, x); }

    constexpr D3D12_CULL_MODE D3D12CullModeMap[]
    {
        D3D12_CULL_MODE_NONE,   // None
        D3D12_CULL_MODE_FRONT,  // Front
        D3D12_CULL_MODE_BACK,   // Back
    };
    D3D12_CULL_MODE ToDxCullMode(CullMode x) { return _ToDxType(D3D12CullModeMap, x); }

    constexpr D3D12_STENCIL_OP D3D12StencilOpMap[]
    {
        D3D12_STENCIL_OP_KEEP,      // Keep
        D3D12_STENCIL_OP_ZERO,      // Zero
        D3D12_STENCIL_OP_REPLACE,   // Replace
        D3D12_STENCIL_OP_INCR_SAT,  // IncClamp
        D3D12_STENCIL_OP_DECR_SAT,  // DecClamp
        D3D12_STENCIL_OP_INVERT,    // Invert
        D3D12_STENCIL_OP_INCR,      // IncWrap
        D3D12_STENCIL_OP_DECR,      // DecWrap
    };
    D3D12_STENCIL_OP ToDxStencilOp(StencilOp x) { return _ToDxType(D3D12StencilOpMap, x); }

    constexpr D3D12_COMPARISON_FUNC D3D12ComparisonFuncMap[]
    {
        D3D12_COMPARISON_FUNC_NEVER,            // Never
        D3D12_COMPARISON_FUNC_LESS,             // Less
        D3D12_COMPARISON_FUNC_EQUAL,            // Equal
        D3D12_COMPARISON_FUNC_LESS_EQUAL,       // LessEqual
        D3D12_COMPARISON_FUNC_GREATER,          // Greater
        D3D12_COMPARISON_FUNC_NOT_EQUAL,        // NotEqual
        D3D12_COMPARISON_FUNC_GREATER_EQUAL,    // GreaterEqual
        D3D12_COMPARISON_FUNC_ALWAYS,           // Always
    };
    D3D12_COMPARISON_FUNC ToDxComparisonFunc(ComparisonFunc x) { return _ToDxType(D3D12ComparisonFuncMap, x); }

    constexpr D3D12_FILL_MODE D3D12FillModeMap[]
    {
        D3D12_FILL_MODE_SOLID,      // Solid
        D3D12_FILL_MODE_WIREFRAME,  // Wireframe
    };
    D3D12_FILL_MODE ToDxFillMode(FillMode x) { return _ToDxType(D3D12FillModeMap, x); }

    constexpr D3D12_FILTER_TYPE D3D12FilterTypeMap[]
    {
        D3D12_FILTER_TYPE_POINT,    // Nearest
        D3D12_FILTER_TYPE_LINEAR,   // Linear
    };
    D3D12_FILTER_TYPE ToDxFilterType(Filter x) { return _ToDxType(D3D12FilterTypeMap, x); }

    constexpr D3D12_FILTER_REDUCTION_TYPE D3D12FilterReductionTypeMap[]
    {
        D3D12_FILTER_REDUCTION_TYPE_STANDARD,   // Standard
        D3D12_FILTER_REDUCTION_TYPE_COMPARISON, // Compare
        D3D12_FILTER_REDUCTION_TYPE_MINIMUM,    // Min
        D3D12_FILTER_REDUCTION_TYPE_MAXIMUM,    // Max
    };
    D3D12_FILTER_REDUCTION_TYPE ToDxFilterReductionType(FilterReduce x) { return _ToDxType(D3D12FilterReductionTypeMap, x); }

    constexpr D3D12_TEXTURE_ADDRESS_MODE D3D12TextureAddressModeMap[]
    {
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,      // Border
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,       // Clamp
        D3D12_TEXTURE_ADDRESS_MODE_MIRROR,      // Mirror
        D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE, // MirrorOnce
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,        // Wrap
    };
    D3D12_TEXTURE_ADDRESS_MODE ToDxAddressMode(AddressMode x) { return _ToDxType(D3D12TextureAddressModeMap, x); }

    constexpr DXGI_COLOR_SPACE_TYPE DxgiColorSpaceTypeMap[]
    {
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,    // sRGB
        DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709,    // scRGB
        DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, // HDR10_PQ
    };
    DXGI_COLOR_SPACE_TYPE ToDxColorSpace(ColorSpace x) { return _ToDxType(DxgiColorSpaceTypeMap, x); }

    ColorSpace FromDxColorSpace(DXGI_COLOR_SPACE_TYPE x)
    {
        switch (x)
        {
            case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709: return ColorSpace::sRGB;
            case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709: return ColorSpace::scRGB;
            case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020: return ColorSpace::HDR10_PQ;
            default:
                HE_ASSERT(false, HE_MSG("Unknown DXGI Color Space"));
                HE_UNREACHABLE();
        }
    };

    constexpr DXGI_SCALING DxgiScalingMap[]
    {
        DXGI_SCALING_NONE,                  // None
        DXGI_SCALING_STRETCH,               // Stretch
        DXGI_SCALING_ASPECT_RATIO_STRETCH,  // AspectRatioStretch
    };
    DXGI_SCALING ToDxScaling(SwapChainScaling x) { return _ToDxType(DxgiScalingMap, x); }

    constexpr DXGI_FORMAT DxgiIndexFormatMap[]
    {
        DXGI_FORMAT_R16_UINT, // Uint16
        DXGI_FORMAT_R32_UINT, // Uint32
    };
    DXGI_FORMAT ToDxIndexFormat(IndexType x) { return _ToDxType(DxgiIndexFormatMap, x); }

    DXGI_FORMAT ToDxSwapChainFormat(Format x)
    {
        switch (x)
        {
            case Format::RGBA8Unorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::RGBA8Unorm_sRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::BGRA8Unorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case Format::BGRA8Unorm_sRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case Format::RGBA16Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::RGB10A2Unorm: return DXGI_FORMAT_R10G10B10A2_UNORM;
            default:
                HE_ASSERT(false, HE_MSG("Invalid format for Swap Chain: {}", x));
                HE_UNREACHABLE();
        }
    }

    DXGI_FORMAT ToDxSwapChainViewFormat(Format x)
    {
        switch (x)
        {
            case Format::RGBA8Unorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::RGBA8Unorm_sRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case Format::BGRA8Unorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case Format::BGRA8Unorm_sRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case Format::RGBA16Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::RGB10A2Unorm: return DXGI_FORMAT_R10G10B10A2_UNORM;
            default:
                HE_ASSERT(false, HE_MSG("Invalid format for Swap Chain View: {}", x));
                HE_UNREACHABLE();
        }
    }

    Format FromDxSwapChainViewFormat(DXGI_FORMAT x)
    {
        switch (x)
        {
            case DXGI_FORMAT_R8G8B8A8_UNORM: return Format::RGBA8Unorm;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return Format::RGBA8Unorm_sRGB;
            case DXGI_FORMAT_B8G8R8A8_UNORM: return Format::BGRA8Unorm;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return Format::BGRA8Unorm_sRGB;
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return Format::RGBA16Float;
            case DXGI_FORMAT_R10G10B10A2_UNORM: return Format::RGB10A2Unorm;
            default:
                HE_ASSERT(false, HE_MSG("Invalid DXGI Swap Chain format: {}", x));
                HE_UNREACHABLE();
        }
    }

    constexpr DXGI_FORMAT DxgiFormatMap[]
    {
        DXGI_FORMAT_UNKNOWN,                // Invalid

        // 8-bit formats
        DXGI_FORMAT_R8_UNORM,               // R8Unorm
        DXGI_FORMAT_R8_SNORM,               // R8Snorm
        DXGI_FORMAT_R8_UINT,                // R8Uint
        DXGI_FORMAT_R8_SINT,                // R8Sint
        DXGI_FORMAT_A8_UNORM,               // A8Unorm

        // 16-bit formats
        DXGI_FORMAT_R16_UNORM,              // R16Unorm
        DXGI_FORMAT_R16_SNORM,              // R16Snorm
        DXGI_FORMAT_R16_UINT,               // R16Uint
        DXGI_FORMAT_R16_SINT,               // R16Sint
        DXGI_FORMAT_R16_FLOAT,              // R16Float
        DXGI_FORMAT_R8G8_UNORM,             // RG8Unorm
        DXGI_FORMAT_R8G8_SNORM,             // RG8Snorm
        DXGI_FORMAT_R8G8_UINT,              // RG8Uint
        DXGI_FORMAT_R8G8_SINT,              // RG8Sint

        // 16-bit packed formats
        DXGI_FORMAT_B4G4R4A4_UNORM,         // BGRA4Unorm

        // 32-bit formats
        DXGI_FORMAT_R32_UINT,               // R32Uint
        DXGI_FORMAT_R32_SINT,               // R32Sint
        DXGI_FORMAT_R32_FLOAT,              // R32Float
        DXGI_FORMAT_R16G16_UNORM,           // RG16Unorm
        DXGI_FORMAT_R16G16_SNORM,           // RG16Snorm
        DXGI_FORMAT_R16G16_UINT,            // RG16Uint
        DXGI_FORMAT_R16G16_SINT,            // RG16Sint
        DXGI_FORMAT_R16G16_FLOAT,           // RG16Float
        DXGI_FORMAT_R8G8B8A8_UNORM,         // RGBA8Unorm
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    // RGBA8Unorm_sRGB
        DXGI_FORMAT_R8G8B8A8_SNORM,         // RGBA8Snorm
        DXGI_FORMAT_R8G8B8A8_UINT,          // RGBA8Uint
        DXGI_FORMAT_R8G8B8A8_SINT,          // RGBA8Sint
        DXGI_FORMAT_B8G8R8A8_UNORM,         // BGRA8Unorm
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,    // BGRA8Unorm_sRGB

        // 32-bit packed formats
        DXGI_FORMAT_R10G10B10A2_UNORM,      // RGB10A2Unorm
        DXGI_FORMAT_R10G10B10A2_UINT,       // RGB10A2Uint
        DXGI_FORMAT_R11G11B10_FLOAT,        // RG11B10Float
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP,     // RGB9E5SharedExp

        // 64-bit formats
        DXGI_FORMAT_R32G32_UINT,            // RG32Uint
        DXGI_FORMAT_R32G32_SINT,            // RG32Sint
        DXGI_FORMAT_R32G32_FLOAT,           // RG32Float
        DXGI_FORMAT_R16G16B16A16_UNORM,     // RGBA16Unorm
        DXGI_FORMAT_R16G16B16A16_SNORM,     // RGBA16Snorm
        DXGI_FORMAT_R16G16B16A16_UINT,      // RGBA16Uint
        DXGI_FORMAT_R16G16B16A16_SINT,      // RGBA16Sint
        DXGI_FORMAT_R16G16B16A16_FLOAT,     // RGBA16Float

        // 96-bit formats
        DXGI_FORMAT_R32G32B32_UINT,         // RGB32Uint
        DXGI_FORMAT_R32G32B32_SINT,         // RGB32Sint
        DXGI_FORMAT_R32G32B32_FLOAT,        // RGB32Float

        // 128-bit formats
        DXGI_FORMAT_R32G32B32A32_UINT,      // RGBA32Uint
        DXGI_FORMAT_R32G32B32A32_SINT,      // RGBA32Sint
        DXGI_FORMAT_R32G32B32A32_FLOAT,     // RGBA32Float

        // Compressed BC formats
        DXGI_FORMAT_BC1_UNORM,              // BC1_RGBA
        DXGI_FORMAT_BC1_UNORM_SRGB,         // BC1_RGBA_sRGB
        DXGI_FORMAT_BC2_UNORM,              // BC2_RGBA
        DXGI_FORMAT_BC2_UNORM_SRGB,         // BC2_RGBA_sRGB
        DXGI_FORMAT_BC3_UNORM,              // BC3_RGBA
        DXGI_FORMAT_BC3_UNORM_SRGB,         // BC3_RGBA_sRGB
        DXGI_FORMAT_BC4_UNORM,              // BC4_RUnorm
        DXGI_FORMAT_BC4_SNORM,              // BC4_RSnorm
        DXGI_FORMAT_BC5_UNORM,              // BC5_RGUnorm
        DXGI_FORMAT_BC5_SNORM,              // BC5_RGSnorm
        DXGI_FORMAT_BC6H_SF16,              // BC6H_RGBFloat
        DXGI_FORMAT_BC6H_UF16,              // BC6H_RGBUfloat
        DXGI_FORMAT_BC7_UNORM,              // BC7_RGBA
        DXGI_FORMAT_BC7_UNORM_SRGB,         // BC7_RGBA_sRGB

        // Depth/stencil formats
        DXGI_FORMAT_D16_UNORM,              // Depth16Unorm
        DXGI_FORMAT_D32_FLOAT,              // Depth32Float
        DXGI_FORMAT_D24_UNORM_S8_UINT,      // Depth24Unorm_Stencil8
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT,   // Depth32Float_Stencil8
    };
    DXGI_FORMAT ToDxFormat(Format x) { return _ToDxType(DxgiFormatMap, x); }

    constexpr D3D12_STATIC_BORDER_COLOR D3D12StaticBorderColorMap[]
    {
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,         // Black
        D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,    // TransparentBlack
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,         // White
    };
    D3D12_STATIC_BORDER_COLOR ToDxStaticBorderColor(BorderColor x) { return _ToDxType(D3D12StaticBorderColorMap, x); }

    constexpr float BorderColorMap[][4]
    {
        { 0.0f, 0.0f, 0.0f, 0.0f },        // TransparentBlack
        { 0.0f, 0.0f, 0.0f, 1.0f },        // OpaqueBlack
        { 1.0f, 1.0f, 1.0f, 1.0f },        // OpaqueWhite
    };
    auto ToDxBorderColor(BorderColor x) -> const float(&)[4] { return _ToDxType(BorderColorMap, x); }
}

#endif
