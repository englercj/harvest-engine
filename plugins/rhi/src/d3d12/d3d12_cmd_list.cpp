// Copyright Chad Engler

#include "d3d12_cmd_list.h"

#include "d3d12_common.h"
#include "d3d12_device.h"
#include "d3d12_formats.h"
#include "d3d12_resources.h"

#include "he/core/assert.h"
#include "he/math/vec4.h"
#include "he/rhi/utils.h"

#if HE_RHI_ENABLE_D3D12

#if HE_RHI_ENABLE_PIX
    // X is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
    #pragma warning(push)
    #pragma warning(disable : 4668)

    #include <pix3.h>

    #pragma warning(pop)
#endif

namespace he::rhi::d3d12
{
    // --------------------------------------------------------------------------------------------
    // Shared Helpers

    static uint32_t CalcSubresource(uint32_t mip, uint32_t layer, uint32_t mipCount)
    {
        return mip + (layer * mipCount);
    }


    static void CopyBufferToBuffer(ID3D12GraphicsCommandList* d3dCmdList, const Buffer* src_, const Buffer* dst_, const BufferCopy* region)
    {
        const BufferImpl* src = static_cast<const BufferImpl*>(src_);
        const BufferImpl* dst = static_cast<const BufferImpl*>(dst_);

        if (!region)
        {
            d3dCmdList->CopyResource(dst->d3dResource, src->d3dResource);
            return;
        }

        d3dCmdList->CopyBufferRegion(dst->d3dResource, region->dstOffset, src->d3dResource, region->srcOffset, region->size);
    }

    static void CopyTextureToTexture(ID3D12GraphicsCommandList* d3dCmdList, const Texture* src_, const Texture* dst_, const TextureCopy* region)
    {
        const TextureImpl* src = static_cast<const TextureImpl*>(src_);
        const TextureImpl* dst = static_cast<const TextureImpl*>(dst_);

        if (!region)
        {
            d3dCmdList->CopyResource(dst->d3dResource, src->d3dResource);
            return;
        }

        D3D12_BOX srcBox{};
        srcBox.left = region->srcOffset.x;
        srcBox.top = region->srcOffset.y;
        srcBox.front = region->srcOffset.z;
        srcBox.right = region->srcOffset.x + region->srcSize.x;
        srcBox.bottom = region->srcOffset.y + region->srcSize.y;
        srcBox.back = region->srcOffset.z + region->srcSize.z;

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = src->d3dResource;
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = CalcSubresource(region->srcMip, region->srcLayer, src->mipCount);

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = dst->d3dResource;
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = CalcSubresource(region->dstMip, region->dstLayer, dst->mipCount);

        d3dCmdList->CopyTextureRegion(
            &dstLoc,
            region->dstOffset.x,
            region->dstOffset.y,
            region->dstOffset.z,
            &srcLoc,
            &srcBox);
    }

    static void CopyBufferToTexture(ID3D12GraphicsCommandList* d3dCmdList, const Buffer* src_, const Texture* dst_, const BufferTextureCopy& region)
    {
        const BufferImpl* src = static_cast<const BufferImpl*>(src_);
        const TextureImpl* dst = static_cast<const TextureImpl*>(dst_);

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = src->d3dResource;
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint.Offset = region.bufferOffset;
        srcLoc.PlacedFootprint.Footprint.Format = ToDxFormat(dst->format);
        srcLoc.PlacedFootprint.Footprint.Width = region.textureSize.x;
        srcLoc.PlacedFootprint.Footprint.Height = region.textureSize.y;
        srcLoc.PlacedFootprint.Footprint.Depth = region.textureSize.z;
        srcLoc.PlacedFootprint.Footprint.RowPitch = region.bufferRowPitch;

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = dst->d3dResource;
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = CalcSubresource(region.textureMip, region.textureLayer, dst->mipCount);

        d3dCmdList->CopyTextureRegion(
            &dstLoc,
            region.textureOffset.x,
            region.textureOffset.y,
            region.textureOffset.z,
            &srcLoc,
            nullptr);
    }

    static void CopyTextureToBuffer(ID3D12GraphicsCommandList* d3dCmdList, const Texture* src_, const Buffer* dst_, const BufferTextureCopy& region)
    {
        const TextureImpl* src = static_cast<const TextureImpl*>(src_);
        const BufferImpl* dst = static_cast<const BufferImpl*>(dst_);

        D3D12_TEXTURE_COPY_LOCATION srcLoc;
        srcLoc.pResource = src->d3dResource;
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = CalcSubresource(region.textureMip, region.textureLayer, src->mipCount);

        D3D12_TEXTURE_COPY_LOCATION dstLoc;
        dstLoc.pResource = dst->d3dResource;
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLoc.PlacedFootprint.Offset = region.bufferOffset;
        dstLoc.PlacedFootprint.Footprint.Format = ToDxFormat(src->format);
        dstLoc.PlacedFootprint.Footprint.Width = region.textureSize.x;
        dstLoc.PlacedFootprint.Footprint.Height = region.textureSize.y;
        dstLoc.PlacedFootprint.Footprint.Depth = region.textureSize.z;
        dstLoc.PlacedFootprint.Footprint.RowPitch = region.bufferRowPitch;

        d3dCmdList->CopyTextureRegion(
            &dstLoc,
            region.textureOffset.x,
            region.textureOffset.y,
            region.textureOffset.z,
            &srcLoc,
            nullptr);
    }

    static void AddTransitionBarrier(ID3D12GraphicsCommandList* d3dCmdList, ID3D12Resource* d3dResource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState, uint32_t subresource)
    {
        static_assert(AllSubresources == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, "");

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = d3dResource;
        barrier.Transition.Subresource = subresource;
        barrier.Transition.StateBefore = oldState;
        barrier.Transition.StateAfter = newState;
        d3dCmdList->ResourceBarrier(1, &barrier);
    }

    static void AddRWBarrier(ID3D12GraphicsCommandList* d3dCmdList, ID3D12Resource* d3dResource)
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = d3dResource;
        d3dCmdList->ResourceBarrier(1, &barrier);
    }

    // --------------------------------------------------------------------------------------------
    // Copy Command List

    CopyCmdListImpl::~CopyCmdListImpl() noexcept
    {
        HE_DX_SAFE_RELEASE(m_d3dCmdList);
    }

    Result CopyCmdListImpl::Initialize(DeviceImpl* device, const CmdListDesc& desc)
    {
        CmdAllocatorImpl* alloc = static_cast<CmdAllocatorImpl*>(desc.alloc);

        ID3D12GraphicsCommandList* d3dCmdList = nullptr;
        HRESULT hr = device->m_d3dDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_COPY,
            alloc->d3dCmdAllocator,
            nullptr,
            IID_PPV_ARGS(&d3dCmdList));

        if (FAILED(hr))
            return device->MakeResult(hr);

        d3dCmdList->Close();

        m_device = device;
        m_d3dCmdList = d3dCmdList;
        return Result::Success;
    }

    Result CopyCmdListImpl::Begin(CmdAllocator* alloc_)
    {
        CmdAllocatorImpl* alloc = static_cast<CmdAllocatorImpl*>(alloc_);
        HRESULT hr = m_d3dCmdList->Reset(alloc->d3dCmdAllocator, nullptr);
        return m_device->MakeResult(hr);
    }

    Result CopyCmdListImpl::End()
    {
        HRESULT hr = m_d3dCmdList->Close();
        return m_device->MakeResult(hr);
    }

    void CopyCmdListImpl::BeginGroup(const char* msg)
    {
        HE_UNUSED(msg);
    #if HE_RHI_ENABLE_PIX
        PIXBeginEvent(m_d3dCmdList, 1, msg);
    #endif
    }

    void CopyCmdListImpl::EndGroup()
    {
    #if HE_RHI_ENABLE_PIX
        PIXEndEvent(m_d3dCmdList);
    #endif
    }

    void CopyCmdListImpl::SetMarker(const char* msg)
    {
        HE_UNUSED(msg);
    #if HE_RHI_ENABLE_PIX
        PIXSetMarker(m_d3dCmdList, 1, msg);
    #endif
    }

    void CopyCmdListImpl::Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region)
    {
        CopyBufferToBuffer(m_d3dCmdList, src, dst, region);
    }

    void CopyCmdListImpl::Copy(const Texture* src, const Texture* dst, const TextureCopy* region)
    {
        CopyTextureToTexture(m_d3dCmdList, src, dst, region);
    }

    void CopyCmdListImpl::Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region)
    {
        CopyBufferToTexture(m_d3dCmdList, src, dst, region);
    }

    void CopyCmdListImpl::Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region)
    {
        CopyTextureToBuffer(m_d3dCmdList, src, dst, region);
    }

    // --------------------------------------------------------------------------------------------
    // Compute Command List

    ComputeCmdListImpl::~ComputeCmdListImpl() noexcept
    {
        HE_DX_SAFE_RELEASE(m_d3dCmdList);
    }

    Result ComputeCmdListImpl::Initialize(DeviceImpl* device, const CmdListDesc& desc)
    {
        CmdAllocatorImpl* alloc = static_cast<CmdAllocatorImpl*>(desc.alloc);

        ID3D12GraphicsCommandList* d3dCmdList = nullptr;
        HRESULT hr = device->m_d3dDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_COMPUTE,
            alloc->d3dCmdAllocator,
            nullptr,
            IID_PPV_ARGS(&d3dCmdList));

        if (FAILED(hr))
            return device->MakeResult(hr);

        d3dCmdList->Close();

        m_device = device;
        m_d3dCmdList = d3dCmdList;
        return Result::Success;
    }

    Result ComputeCmdListImpl::Begin(CmdAllocator* alloc_)
    {
        CmdAllocatorImpl* alloc = static_cast<CmdAllocatorImpl*>(alloc_);
        HRESULT hr = m_d3dCmdList->Reset(alloc->d3dCmdAllocator, nullptr);
        if (FAILED(hr))
            return m_device->MakeResult(hr);

        ID3D12DescriptorHeap* heaps[] =
        {
            m_device->m_gpuGeneralPool.GetHeap(),
            m_device->m_gpuSamplerPool.GetHeap(),
        };
        m_d3dCmdList->SetDescriptorHeaps(2, heaps);

        return Result::Success;
    }

    Result ComputeCmdListImpl::End()
    {
        HRESULT hr = m_d3dCmdList->Close();
        return m_device->MakeResult(hr);
    }

    void ComputeCmdListImpl::BeginGroup(const char* msg)
    {
        HE_UNUSED(msg);
    #if HE_RHI_ENABLE_PIX
        PIXBeginEvent(m_d3dCmdList, 1, msg);
    #endif
    }

    void ComputeCmdListImpl::EndGroup()
    {
    #if HE_RHI_ENABLE_PIX
        PIXEndEvent(m_d3dCmdList);
    #endif
    }

    void ComputeCmdListImpl::SetMarker(const char* msg)
    {
        HE_UNUSED(msg);
    #if HE_RHI_ENABLE_PIX
        PIXSetMarker(m_d3dCmdList, 1, msg);
    #endif
    }

    void ComputeCmdListImpl::Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region)
    {
        CopyBufferToBuffer(m_d3dCmdList, src, dst, region);
    }

    void ComputeCmdListImpl::Copy(const Texture* src, const Texture* dst, const TextureCopy* region)
    {
        CopyTextureToTexture(m_d3dCmdList, src, dst, region);
    }

    void ComputeCmdListImpl::Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region)
    {
        CopyBufferToTexture(m_d3dCmdList, src, dst, region);
    }

    void ComputeCmdListImpl::Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region)
    {
        CopyTextureToBuffer(m_d3dCmdList, src, dst, region);
    }

    void ComputeCmdListImpl::Clear(const RWBufferView* view_, const Vec4f& values)
    {
        const RWBufferViewImpl* view = static_cast<const RWBufferViewImpl*>(view_);
        m_d3dCmdList->ClearUnorderedAccessViewFloat(view->d3dGpuHandle, view->d3dCpuHandle, view->buffer->d3dResource, GetPointer(values), 0, nullptr);
    }

    void ComputeCmdListImpl::Clear(const RWBufferView* view_, const Vec4u& values)
    {
        const RWBufferViewImpl* view = static_cast<const RWBufferViewImpl*>(view_);
        m_d3dCmdList->ClearUnorderedAccessViewUint(view->d3dGpuHandle, view->d3dCpuHandle, view->buffer->d3dResource, GetPointer(values), 0, nullptr);
    }

    void ComputeCmdListImpl::Clear(const RWTextureView* view_, const Vec4f& values)
    {
        const RWTextureViewImpl* view = static_cast<const RWTextureViewImpl*>(view_);
        m_d3dCmdList->ClearUnorderedAccessViewFloat(view->d3dGpuHandle, view->d3dCpuHandle, view->texture->d3dResource, GetPointer(values), 0, nullptr);
    }

    void ComputeCmdListImpl::Clear(const RWTextureView* view_, const Vec4u& values)
    {
        const RWTextureViewImpl* view = static_cast<const RWTextureViewImpl*>(view_);
        m_d3dCmdList->ClearUnorderedAccessViewUint(view->d3dGpuHandle, view->d3dCpuHandle, view->texture->d3dResource, GetPointer(values), 0, nullptr);
    }

    void ComputeCmdListImpl::SetComputeRootSignature(const RootSignature* signature_)
    {
        const RootSignatureImpl* signature = static_cast<const RootSignatureImpl*>(signature_);
        m_computeRootSignature = signature;
        m_d3dCmdList->SetComputeRootSignature(signature->d3dRootSignature);
    }

    void ComputeCmdListImpl::SetComputePipeline(const ComputePipeline* pipeline_)
    {
        const ComputePipelineImpl* pipeline = static_cast<const ComputePipelineImpl*>(pipeline_);
        m_d3dCmdList->SetPipelineState(pipeline->d3dPipeline);
    }

    void ComputeCmdListImpl::SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table_)
    {
        const DescriptorTableImpl* table = static_cast<const DescriptorTableImpl*>(table_);
        m_d3dCmdList->SetComputeRootDescriptorTable(slot, table->d3dGpuStart);
    }

    void ComputeCmdListImpl::SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer_, uint32_t offset)
    {
        const BufferImpl* buffer = static_cast<const BufferImpl*>(buffer_);
        m_d3dCmdList->SetComputeRootConstantBufferView(slot, buffer->gpuAddress + offset);
    }

    void ComputeCmdListImpl::SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset)
    {
        m_d3dCmdList->SetComputeRoot32BitConstant(slot, value, offset);
    }

    void ComputeCmdListImpl::SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset)
    {
        m_d3dCmdList->SetComputeRoot32BitConstants(slot, count, data, offset);
    }

    void ComputeCmdListImpl::Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ)
    {
        m_d3dCmdList->Dispatch(countX, countY, countZ);
    }

    void ComputeCmdListImpl::TransitionBarrier(const Texture* texture_, TextureState oldState, TextureState newState, uint32_t subresource)
    {
        const TextureImpl* texture = static_cast<const TextureImpl*>(texture_);
        AddTransitionBarrier(m_d3dCmdList, texture->d3dResource, ToDxTextureResourceState(oldState), ToDxTextureResourceState(newState), subresource);
    }

    void ComputeCmdListImpl::TransitionBarrier(const Buffer* buffer_, BufferState oldState, BufferState newState)
    {
        const BufferImpl* buffer = static_cast<const BufferImpl*>(buffer_);
        AddTransitionBarrier(m_d3dCmdList, buffer->d3dResource, ToDxBufferResourceState(oldState), ToDxBufferResourceState(newState), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    }

    void ComputeCmdListImpl::RWBarrier(const Buffer* buffer_)
    {
        const BufferImpl* buffer = static_cast<const BufferImpl*>(buffer_);
        AddRWBarrier(m_d3dCmdList, buffer->d3dResource);
    }

    void ComputeCmdListImpl::RWBarrier(const Texture* texture_)
    {
        const TextureImpl* texture = static_cast<const TextureImpl*>(texture_);
        AddRWBarrier(m_d3dCmdList, texture->d3dResource);
    }

    // --------------------------------------------------------------------------------------------
    // Render Command List

    RenderCmdListImpl::~RenderCmdListImpl() noexcept
    {
        HE_DX_SAFE_RELEASE(m_d3dCmdList);
    }

    Result RenderCmdListImpl::Initialize(DeviceImpl* device, const CmdListDesc& desc)
    {
        CmdAllocatorImpl* alloc = static_cast<CmdAllocatorImpl*>(desc.alloc);

        ID3D12GraphicsCommandList* d3dCmdList = nullptr;
        HRESULT hr = device->m_d3dDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            alloc->d3dCmdAllocator,
            nullptr,
            IID_PPV_ARGS(&d3dCmdList));

        if (FAILED(hr))
            return device->MakeResult(hr);

        d3dCmdList->Close();

        m_device = device;
        m_d3dCmdList = d3dCmdList;
        return Result::Success;
    }

    Result RenderCmdListImpl::Begin(CmdAllocator* alloc_)
    {
        CmdAllocatorImpl* alloc = static_cast<CmdAllocatorImpl*>(alloc_);
        HRESULT hr = m_d3dCmdList->Reset(alloc->d3dCmdAllocator, nullptr);
        if (FAILED(hr))
            return m_device->MakeResult(hr);

        ID3D12DescriptorHeap* heaps[] =
        {
            m_device->m_gpuGeneralPool.GetHeap(),
            m_device->m_gpuSamplerPool.GetHeap(),
        };
        m_d3dCmdList->SetDescriptorHeaps(2, heaps);

        return Result::Success;
    }

    Result RenderCmdListImpl::End()
    {
        HRESULT hr = m_d3dCmdList->Close();
        return m_device->MakeResult(hr);
    }

    void RenderCmdListImpl::BeginGroup(const char* msg)
    {
        HE_UNUSED(msg);
    #if HE_RHI_ENABLE_PIX
        PIXBeginEvent(m_d3dCmdList, 1, msg);
    #endif
    }

    void RenderCmdListImpl::EndGroup()
    {
    #if HE_RHI_ENABLE_PIX
        PIXEndEvent(m_d3dCmdList);
    #endif
    }

    void RenderCmdListImpl::SetMarker(const char* msg)
    {
        HE_UNUSED(msg);
    #if HE_RHI_ENABLE_PIX
        PIXSetMarker(m_d3dCmdList, 1, msg);
    #endif
    }

    void RenderCmdListImpl::Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region)
    {
        CopyBufferToBuffer(m_d3dCmdList, src, dst, region);
    }

    void RenderCmdListImpl::Copy(const Texture* src, const Texture* dst, const TextureCopy* region)
    {
        CopyTextureToTexture(m_d3dCmdList, src, dst, region);
    }

    void RenderCmdListImpl::Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region)
    {
        CopyBufferToTexture(m_d3dCmdList, src, dst, region);
    }

    void RenderCmdListImpl::Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region)
    {
        CopyTextureToBuffer(m_d3dCmdList, src, dst, region);
    }

    void RenderCmdListImpl::Clear(const RWBufferView* view_, const Vec4f& values)
    {
        const RWBufferViewImpl* view = static_cast<const RWBufferViewImpl*>(view_);
        m_d3dCmdList->ClearUnorderedAccessViewFloat(view->d3dGpuHandle, view->d3dCpuHandle, view->buffer->d3dResource, GetPointer(values), 0, nullptr);
    }

    void RenderCmdListImpl::Clear(const RWBufferView* view_, const Vec4u& values)
    {
        const RWBufferViewImpl* view = static_cast<const RWBufferViewImpl*>(view_);
        m_d3dCmdList->ClearUnorderedAccessViewUint(view->d3dGpuHandle, view->d3dCpuHandle, view->buffer->d3dResource, GetPointer(values), 0, nullptr);
    }

    void RenderCmdListImpl::Clear(const RWTextureView* view_, const Vec4f& values)
    {
        const RWTextureViewImpl* view = static_cast<const RWTextureViewImpl*>(view_);
        m_d3dCmdList->ClearUnorderedAccessViewFloat(view->d3dGpuHandle, view->d3dCpuHandle, view->texture->d3dResource, GetPointer(values), 0, nullptr);
    }

    void RenderCmdListImpl::Clear(const RWTextureView* view_, const Vec4u& values)
    {
        const RWTextureViewImpl* view = static_cast<const RWTextureViewImpl*>(view_);
        m_d3dCmdList->ClearUnorderedAccessViewUint(view->d3dGpuHandle, view->d3dCpuHandle, view->texture->d3dResource, GetPointer(values), 0, nullptr);
    }

    void RenderCmdListImpl::Clear(const RenderTargetView* rtv_, const Vec4f& values)
    {
        const RenderTargetViewImpl* rtv = static_cast<const RenderTargetViewImpl*>(rtv_);
        HE_ASSERT(!IsDepthFormat(rtv->format));
        m_d3dCmdList->ClearRenderTargetView(rtv->d3dCpuHandle, GetPointer(values), 0, nullptr);
    }

    void RenderCmdListImpl::Clear(const RenderTargetView* rtv_, ClearFlag flags, float depthValue, uint8_t stencilValue)
    {
        if (flags == ClearFlag::None)
            return;

        const RenderTargetViewImpl* rtv = static_cast<const RenderTargetViewImpl*>(rtv_);
        HE_ASSERT(IsDepthFormat(rtv->format));

        D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAGS(0);
        if (HasFlag(flags, ClearFlag::Depth))
            clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
        if (HasFlag(flags, ClearFlag::Stencil))
            clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

        HE_ASSERT(clearFlags > D3D12_CLEAR_FLAGS(0));

        m_d3dCmdList->ClearDepthStencilView(rtv->d3dCpuHandle, clearFlags, depthValue, stencilValue, 0, nullptr);
    }

    void RenderCmdListImpl::SetComputeRootSignature(const RootSignature* signature_)
    {
        const RootSignatureImpl* signature = static_cast<const RootSignatureImpl*>(signature_);
        m_computeRootSignature = signature;
        m_d3dCmdList->SetComputeRootSignature(signature->d3dRootSignature);
    }

    void RenderCmdListImpl::SetComputePipeline(const ComputePipeline* pipeline_)
    {
        const ComputePipelineImpl* pipeline = static_cast<const ComputePipelineImpl*>(pipeline_);
        m_d3dCmdList->SetPipelineState(pipeline->d3dPipeline);
    }

    void RenderCmdListImpl::SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table_)
    {
        const DescriptorTableImpl* table = static_cast<const DescriptorTableImpl*>(table_);
        m_d3dCmdList->SetComputeRootDescriptorTable(slot, table->d3dGpuStart);
    }

    void RenderCmdListImpl::SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer_, uint32_t offset)
    {
        const BufferImpl* buffer = static_cast<const BufferImpl*>(buffer_);
        m_d3dCmdList->SetComputeRootConstantBufferView(slot, buffer->gpuAddress + offset);
    }

    void RenderCmdListImpl::SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset)
    {
        m_d3dCmdList->SetComputeRoot32BitConstant(slot, value, offset);
    }

    void RenderCmdListImpl::SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset)
    {
        m_d3dCmdList->SetComputeRoot32BitConstants(slot, count, data, offset);
    }

    void RenderCmdListImpl::Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ)
    {
        m_d3dCmdList->Dispatch(countX, countY, countZ);
    }

    void RenderCmdListImpl::TransitionBarrier(const Texture* texture_, TextureState oldState, TextureState newState, uint32_t subresource)
    {
        const TextureImpl* texture = static_cast<const TextureImpl*>(texture_);
        AddTransitionBarrier(m_d3dCmdList, texture->d3dResource, ToDxTextureResourceState(oldState), ToDxTextureResourceState(newState), subresource);
    }

    void RenderCmdListImpl::TransitionBarrier(const Buffer* buffer_, BufferState oldState, BufferState newState)
    {
        const BufferImpl* buffer = static_cast<const BufferImpl*>(buffer_);
        AddTransitionBarrier(m_d3dCmdList, buffer->d3dResource, ToDxBufferResourceState(oldState), ToDxBufferResourceState(newState), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    }

    void RenderCmdListImpl::RWBarrier(const Buffer* buffer_)
    {
        const BufferImpl* buffer = static_cast<const BufferImpl*>(buffer_);
        AddRWBarrier(m_d3dCmdList, buffer->d3dResource);
    }

    void RenderCmdListImpl::RWBarrier(const Texture* texture_)
    {
        const TextureImpl* texture = static_cast<const TextureImpl*>(texture_);
        AddRWBarrier(m_d3dCmdList, texture->d3dResource);
    }

    void RenderCmdListImpl::BeginRenderPass(const RenderPassDesc& desc)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE colorDescs[MaxColorAttachments];
        const D3D12_CPU_DESCRIPTOR_HANDLE* depthDesc = nullptr;

        m_colorAttachmentCount = desc.colorAttachmentCount;

        // Discard/clear color targets
        for (uint32_t i = 0; i < desc.colorAttachmentCount; ++i)
        {
            const ColorAttachment& attachment = desc.colorAttachments[i];
            const RenderTargetViewImpl* rtv = static_cast<const RenderTargetViewImpl*>(attachment.view);
            HE_ASSERT(!IsDepthFormat(rtv->format));

            m_colorAttachments[i] = attachment;
            colorDescs[i] = rtv->d3dCpuHandle;

            if (attachment.state != TextureState::RenderTarget)
                TransitionBarrier(rtv->texture, attachment.state, TextureState::RenderTarget, AllSubresources);

            if (attachment.action.load == LoadOp::DontCare)
            {
                D3D12_DISCARD_REGION region{};
                region.FirstSubresource = rtv->firstSubresource;
                region.NumSubresources = 1;
                m_d3dCmdList->DiscardResource(rtv->texture->d3dResource, &region);
            }
            else if (attachment.action.load == LoadOp::Clear)
            {
                m_d3dCmdList->ClearRenderTargetView(rtv->d3dCpuHandle, GetPointer(attachment.action.clearValue), 0, nullptr);
            }
        }

        // Discard/clear depth-stencil target
        if (desc.depthStencilAttachment)
        {
            const DepthStencilAttachment& attachment = *desc.depthStencilAttachment;
            const RenderTargetViewImpl* rtv = static_cast<const RenderTargetViewImpl*>(attachment.view);
            HE_ASSERT(IsDepthFormat(rtv->format));

            m_depthStencilAttachment = attachment;
            m_hasDepthStencilAttachment = true;

            depthDesc = &rtv->d3dCpuHandle;

            if (attachment.state != TextureState::DepthWrite)
                TransitionBarrier(rtv->texture, attachment.state, TextureState::DepthWrite, AllSubresources);

            if (attachment.depthAction.load == LoadOp::DontCare && attachment.stencilAction.load == LoadOp::DontCare)
            {
                D3D12_DISCARD_REGION region{};
                region.FirstSubresource = rtv->firstSubresource;
                region.NumSubresources = 1;
                m_d3dCmdList->DiscardResource(rtv->texture->d3dResource, &region);
            }

            D3D12_CLEAR_FLAGS clearFlags{};
            if (attachment.depthAction.load == LoadOp::Clear)
                clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
            if (attachment.stencilAction.load == LoadOp::Clear)
                clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

            if (clearFlags)
            {
                float depthValue = attachment.depthAction.clearValue;
                uint8_t stencilValue = attachment.stencilAction.clearValue;
                m_d3dCmdList->ClearDepthStencilView(rtv->d3dCpuHandle, clearFlags, depthValue, stencilValue, 0, nullptr);
            }
        }
        else
        {
            m_hasDepthStencilAttachment = false;
        }

        m_d3dCmdList->OMSetRenderTargets(desc.colorAttachmentCount, colorDescs, FALSE, depthDesc);
    }

    void RenderCmdListImpl::EndRenderPass()
    {
        // Discard/resolve color targets
        for (uint32_t i = 0; i < m_colorAttachmentCount; ++i)
        {
            const ColorAttachment& attachment = m_colorAttachments[i];
            const RenderTargetViewImpl* srcView = static_cast<const RenderTargetViewImpl*>(attachment.view);
            HE_ASSERT(!IsDepthFormat(srcView->format));

            if (attachment.action.store == StoreOp::Resolve || attachment.action.store == StoreOp::StoreAndResolve)
            {
                const RenderTargetViewImpl* dstView = static_cast<const RenderTargetViewImpl*>(attachment.action.resolveToView);
                DXGI_FORMAT format = ToDxFormat(dstView->texture->format);
                HE_ASSERT(!IsDepthFormat(dstView->format));

                m_d3dCmdList->ResolveSubresource(
                    dstView->texture->d3dResource,
                    dstView->firstSubresource,
                    srcView->texture->d3dResource,
                    srcView->firstSubresource,
                    format);
            }

            if (attachment.action.store == StoreOp::DontCare || attachment.action.store == StoreOp::Resolve)
            {
                D3D12_DISCARD_REGION region{};
                region.FirstSubresource = srcView->firstSubresource;
                region.NumSubresources = 1;
                m_d3dCmdList->DiscardResource(srcView->texture->d3dResource, &region);
            }

            if (attachment.state != TextureState::RenderTarget)
                TransitionBarrier(srcView->texture, TextureState::RenderTarget, attachment.state, AllSubresources);
        }

        // Discard/resolve depth-stencil target
        if (m_hasDepthStencilAttachment)
        {
            const DepthStencilAttachment& attachment = m_depthStencilAttachment;
            const RenderTargetViewImpl* srcView = static_cast<const RenderTargetViewImpl*>(attachment.view);
            HE_ASSERT(IsDepthFormat(srcView->format));

            if (attachment.depthAction.store == StoreOp::Resolve || attachment.depthAction.store == StoreOp::StoreAndResolve)
            {
                const RenderTargetViewImpl* dstView = static_cast<const RenderTargetViewImpl*>(attachment.depthAction.resolveToView);
                DXGI_FORMAT format = ToDxFormat(dstView->texture->format);
                HE_ASSERT(IsDepthFormat(dstView->format));

                m_d3dCmdList->ResolveSubresource(
                    dstView->texture->d3dResource,
                    dstView->firstSubresource,
                    srcView->texture->d3dResource,
                    srcView->firstSubresource,
                    format);
            }

            bool depthDiscard = attachment.depthAction.store == StoreOp::DontCare || attachment.depthAction.store == StoreOp::Resolve;
            bool stencilDiscard = attachment.stencilAction.store == StoreOp::DontCare || attachment.stencilAction.store == StoreOp::Resolve;

            if (depthDiscard && stencilDiscard)
            {
                D3D12_DISCARD_REGION region{};
                region.FirstSubresource = srcView->firstSubresource;
                region.NumSubresources = 1;
                m_d3dCmdList->DiscardResource(srcView->texture->d3dResource, &region);
            }

            if (attachment.state != TextureState::DepthWrite)
                TransitionBarrier(srcView->texture, TextureState::DepthWrite, attachment.state, AllSubresources);
        }
    }

    void RenderCmdListImpl::Resolve(const RenderTargetView* srcView_, const RenderTargetView* dstView_)
    {
        const RenderTargetViewImpl* srcView = static_cast<const RenderTargetViewImpl*>(srcView_);
        const RenderTargetViewImpl* dstView = static_cast<const RenderTargetViewImpl*>(dstView_);
        HE_ASSERT(IsDepthFormat(srcView->format) == IsDepthFormat(dstView->format));

        DXGI_FORMAT format = ToDxFormat(dstView->texture->format);

        m_d3dCmdList->ResolveSubresource(
            dstView->texture->d3dResource,
            dstView->firstSubresource,
            srcView->texture->d3dResource,
            srcView->firstSubresource,
            format);
    }

    void RenderCmdListImpl::SetRenderRootSignature(const RootSignature* rootSignature_)
    {
        const RootSignatureImpl* rootSignature = static_cast<const RootSignatureImpl*>(rootSignature_);
        m_d3dCmdList->SetGraphicsRootSignature(rootSignature->d3dRootSignature);
    }

    void RenderCmdListImpl::SetRenderPipeline(const RenderPipeline* pipeline_)
    {
        const RenderPipelineImpl* pipeline = static_cast<const RenderPipelineImpl*>(pipeline_);
        m_d3dCmdList->SetPipelineState(pipeline->d3dPipeline);
        m_d3dCmdList->IASetPrimitiveTopology(pipeline->topology);
    }

    void RenderCmdListImpl::SetRenderDescriptorTable(uint32_t slot, const DescriptorTable* table_)
    {
        const DescriptorTableImpl* table = static_cast<const DescriptorTableImpl*>(table_);
        m_d3dCmdList->SetGraphicsRootDescriptorTable(slot, table->d3dGpuStart);
    }

    void RenderCmdListImpl::SetRenderConstantBuffer(uint32_t slot, const Buffer* buffer_, uint32_t offset)
    {
        const BufferImpl* buffer = static_cast<const BufferImpl*>(buffer_);
        m_d3dCmdList->SetGraphicsRootConstantBufferView(slot, buffer->gpuAddress + offset);
    }

    void RenderCmdListImpl::SetRender32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset)
    {
        m_d3dCmdList->SetGraphicsRoot32BitConstant(slot, value, offset);
    }

    void RenderCmdListImpl::SetRender32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset)
    {
        m_d3dCmdList->SetGraphicsRoot32BitConstants(slot, count, data, offset);
    }

    void RenderCmdListImpl::SetIndexBuffer(const Buffer* buffer_, uint32_t offset, uint32_t size, IndexType type)
    {
        const BufferImpl* buffer = static_cast<const BufferImpl*>(buffer_);

        D3D12_INDEX_BUFFER_VIEW d3dView;
        d3dView.BufferLocation = buffer->gpuAddress + offset;
        d3dView.SizeInBytes = size;
        d3dView.Format = ToDxIndexFormat(type);

        m_d3dCmdList->IASetIndexBuffer(&d3dView);
    }

    void RenderCmdListImpl::SetVertexBuffer(uint32_t index, const VertexBufferFormat* vbf, const Buffer* buffer, uint32_t offset, uint32_t size)
    {
        SetVertexBuffers(index, 1, &vbf, &buffer, &offset, &size);
    }

    void RenderCmdListImpl::SetVertexBuffers(uint32_t index, uint32_t count, const VertexBufferFormat* const* vbfs, const Buffer* const* buffers, const uint32_t* offsets, const uint32_t* sizes)
    {
        HE_ASSERT(count <= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

        D3D12_VERTEX_BUFFER_VIEW views[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];

        for (uint32_t i = 0; i < count; ++i)
        {
            const VertexBufferFormatImpl* vbf = static_cast<const VertexBufferFormatImpl*>(vbfs[i]);
            const BufferImpl* buffer = static_cast<const BufferImpl*>(buffers[i]);
            D3D12_VERTEX_BUFFER_VIEW* view = &views[i];
            view->BufferLocation = buffer->gpuAddress + offsets[i];
            view->SizeInBytes = sizes[i];
            view->StrideInBytes = vbf->stride;
        }

        m_d3dCmdList->IASetVertexBuffers(index, count, views);
    }

    void RenderCmdListImpl::SetViewport(const Viewport& viewport)
    {
        D3D12_VIEWPORT vp;
        vp.TopLeftX = viewport.x;
        vp.TopLeftY = viewport.y;
        vp.Width = viewport.width;
        vp.Height = viewport.height;
        vp.MinDepth = viewport.minZ;
        vp.MaxDepth = viewport.maxZ;

        m_d3dCmdList->RSSetViewports(1, &vp);
    }

    void RenderCmdListImpl::SetScissor(const Vec2u& pos, const Vec2u& size)
    {
        D3D12_RECT d3dRect;
        d3dRect.left = pos.x;
        d3dRect.top = pos.y;
        d3dRect.right = pos.x + size.x;
        d3dRect.bottom = pos.y + size.y;

        m_d3dCmdList->RSSetScissorRects(1, &d3dRect);
    }

    void RenderCmdListImpl::SetStencilRef(uint8_t value)
    {
        m_d3dCmdList->OMSetStencilRef(value);
    }

    void RenderCmdListImpl::SetBlendColor(const Vec4f& color)
    {
        m_d3dCmdList->OMSetBlendFactor(GetPointer(color));
    }

    void RenderCmdListImpl::SetDepthBounds(float min, float max)
    {
        ID3D12GraphicsCommandList1* d3dCmdList1 = nullptr;
        if (SUCCEEDED(m_d3dCmdList->QueryInterface(IID_PPV_ARGS(&d3dCmdList1))))
        {
            d3dCmdList1->OMSetDepthBounds(min, max);
            HE_DX_SAFE_RELEASE(d3dCmdList1);
        }
    }

    void RenderCmdListImpl::Draw(const DrawDesc& desc)
    {
        m_d3dCmdList->DrawInstanced(desc.vertexCount, desc.instanceCount, desc.vertexStart, desc.baseInstance);
    }

    void RenderCmdListImpl::DrawIndexed(const DrawIndexedDesc& desc)
    {
        m_d3dCmdList->DrawIndexedInstanced(desc.indexCount, desc.instanceCount, desc.indexStart, desc.baseVertex, desc.baseInstance);
    }
}

#endif
