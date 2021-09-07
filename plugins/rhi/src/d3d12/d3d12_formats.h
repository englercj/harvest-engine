// Copyright Chad Engler

#pragma once

#include "d3d12_common.h"

#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    D3D12_DESCRIPTOR_RANGE_TYPE ToDxRangeType(DescriptorRangeType x);
    D3D12_RESOURCE_STATES ToDxBufferResourceState(BufferState x);
    D3D12_RESOURCE_STATES ToDxTextureResourceState(TextureState x);
    D3D12_HEAP_TYPE ToDxHeapType(HeapType x);
    D3D12_COMMAND_LIST_TYPE ToDxCmdListType(CmdListType x);
    D3D12_RESOURCE_DIMENSION ToDxTextureDimension(TextureType x);
    D3D12_SRV_DIMENSION ToDxSrvDimension(TextureType x);
    D3D12_UAV_DIMENSION ToDxUavDimension(TextureType x);
    D3D12_DSV_DIMENSION ToDxDsvDimension(TextureType x);
    D3D12_RTV_DIMENSION ToDxRtvDimension(TextureType x);
    D3D12_SHADER_VISIBILITY ToDxShaderVisibility(ShaderStage x);
    D3D12_PRIMITIVE_TOPOLOGY ToDxPrimitiveTopology(PrimitiveType x);
    D3D12_PRIMITIVE_TOPOLOGY_TYPE ToDxPrimitiveTopologyType(PrimitiveType x);
    D3D12_BLEND ToDxBlend(BlendFactor x);
    D3D12_BLEND_OP ToDxBlendOp(BlendOp x);
    D3D12_CULL_MODE ToDxCullMode(CullMode x);
    D3D12_STENCIL_OP ToDxStencilOp(StencilOp x);
    D3D12_COMPARISON_FUNC ToDxComparisonFunc(ComparisonFunc x);
    D3D12_FILL_MODE ToDxFillMode(FillMode x);
    D3D12_FILTER_TYPE ToDxFilterType(Filter x);
    D3D12_FILTER_REDUCTION_TYPE ToDxFilterReductionType(FilterReduce x);
    D3D12_TEXTURE_ADDRESS_MODE ToDxAddressMode(AddressMode x);
    DXGI_COLOR_SPACE_TYPE ToDxColorSpace(ColorSpace x);
    DXGI_SCALING ToDxScaling(SwapChainScaling x);
    DXGI_FORMAT ToDxIndexFormat(IndexType x);
    DXGI_FORMAT ToDxSwapChainFormat(Format x);
    DXGI_FORMAT ToDxSwapChainViewFormat(Format x);
    DXGI_FORMAT ToDxFormat(Format x);
    D3D12_STATIC_BORDER_COLOR ToDxStaticBorderColor(BorderColor x);
    auto ToDxBorderColor(BorderColor x) -> const float(&)[4];

    ColorSpace FromDxColorSpace(DXGI_COLOR_SPACE_TYPE x);
    Format FromDxSwapChainViewFormat(DXGI_FORMAT x);
}

#endif
