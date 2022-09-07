// Copyright Chad Engler

#pragma once

#include "null_device.h"

#include "null_cmd_list.h"
#include "null_instance.h"

#include "he/rhi/config.h"
#include "he/rhi/device.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    DeviceImpl::DeviceImpl(InstanceImpl* instance) noexcept
        : m_instance(instance)
    {}

    Result DeviceImpl::CreateBuffer(const BufferDesc& desc, Buffer*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyBuffer(Buffer* buffer)
    {
        HE_UNUSED(buffer);
    }

    Result DeviceImpl::CreateBufferView(const BufferViewDesc& desc, BufferView*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyBufferView(BufferView* view)
    {
        HE_UNUSED(view);
    }

    Result DeviceImpl::CreateRWBufferView(const BufferViewDesc& desc, RWBufferView*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRWBufferView(RWBufferView* view)
    {
        HE_UNUSED(view);
    }

    void*DeviceImpl:: Map(Buffer* buffer, uint32_t offset, uint32_t size)
    {
        HE_UNUSED(buffer, offset, size);
        return nullptr;
    }

    void DeviceImpl::Unmap(Buffer* buffer)
    {
        HE_UNUSED(buffer);
    }

    Result DeviceImpl::CreateCopyCmdList(const CmdListDesc& desc, CopyCmdList*& out)
    {
        HE_UNUSED(desc);
        out = m_instance->GetAllocator().New<CopyCmdListImpl>();
        return Result::Success;
    }

    void DeviceImpl::DestroyCopyCmdList(CopyCmdList* cmdList)
    {
        m_instance->GetAllocator().Delete(cmdList);
    }

    Result DeviceImpl::CreateComputeCmdList(const CmdListDesc& desc, ComputeCmdList*& out)
    {
        HE_UNUSED(desc);
        out = m_instance->GetAllocator().New<ComputeCmdListImpl>();
        return Result::Success;
    }

    void DeviceImpl::DestroyComputeCmdList(ComputeCmdList* cmdList)
    {
        m_instance->GetAllocator().Delete(cmdList);
    }

    Result DeviceImpl::CreateRenderCmdList(const CmdListDesc& desc, RenderCmdList*& out)
    {
        HE_UNUSED(desc);
        out = m_instance->GetAllocator().New<RenderCmdListImpl>();
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderCmdList(RenderCmdList* cmdList)
    {
        m_instance->GetAllocator().Delete(cmdList);
    }

    Result DeviceImpl::CreateCmdAllocator(const CmdAllocatorDesc& desc, CmdAllocator*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyCmdAllocator(CmdAllocator* alloc)
    {
        HE_UNUSED(alloc);
    }

    Result DeviceImpl::ResetCmdAllocator(CmdAllocator* alloc)
    {
        HE_UNUSED(alloc);
        return Result::Success;
    }

    Result DeviceImpl::CreateDescriptorTable(const DescriptorTableDesc& desc, DescriptorTable*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyDescriptorTable(DescriptorTable* table)
    {
        HE_UNUSED(table);
    }

    void DeviceImpl::SetBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const BufferView* const* views)
    {
        HE_UNUSED(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetTextureViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const TextureView* const* views)
    {
        HE_UNUSED(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetRWBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWBufferView* const* views)
    {
        HE_UNUSED(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetRWTextureViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWTextureView* const* views)
    {
        HE_UNUSED(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetConstantBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const ConstantBufferView* const* views)
    {
        HE_UNUSED(table, rangeIndex, descIndex, count, views);
    }

    void DeviceImpl::SetSamplers(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const Sampler* const* samplers)
    {
        HE_UNUSED(table, rangeIndex, descIndex, count, samplers);
    }

    Result DeviceImpl::CreateCpuFence(const CpuFenceDesc& desc, CpuFence*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyCpuFence(CpuFence* fence)
    {
        HE_UNUSED(fence);
    }

    Result DeviceImpl::CreateGpuFence(const GpuFenceDesc& desc, GpuFence*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyGpuFence(GpuFence* fence)
    {
        HE_UNUSED(fence);
    }

    bool DeviceImpl::WaitForFence(const CpuFence* fence, uint32_t timeoutMs)
    {
        HE_UNUSED(fence, timeoutMs);
        return true;
    }

    bool DeviceImpl::IsFenceSignaled(const CpuFence* fence)
    {
        HE_UNUSED(fence);
        return true;
    }

    uint64_t DeviceImpl::GetFenceValue(const GpuFence* fence)
    {
        HE_UNUSED(fence);
        return 0;
    }

    Result DeviceImpl::CreateComputePipeline(const ComputePipelineDesc& desc, ComputePipeline*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyComputePipeline(ComputePipeline* pipeline)
    {
        HE_UNUSED(pipeline);
    }

    Result DeviceImpl::CreateRenderPipeline(const RenderPipelineDesc& desc, RenderPipeline*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderPipeline(RenderPipeline* pipeline)
    {
        HE_UNUSED(pipeline);
    }

    Result DeviceImpl::CreateRootSignature(const RootSignatureDesc& desc, RootSignature*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRootSignature(RootSignature* signature)
    {
        HE_UNUSED(signature);
    }

    Result DeviceImpl::CreateSampler(const SamplerDesc& desc, Sampler*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroySampler(Sampler* sampler)
    {
        HE_UNUSED(sampler);
    }

    Result DeviceImpl::CreateShader(const ShaderDesc& desc, Shader*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyShader(Shader* shader)
    {
        HE_UNUSED(shader);
    }

    Result DeviceImpl::CreateSwapChain(const SwapChainDesc& desc, SwapChain*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroySwapChain(SwapChain* swapChain)
    {
        HE_UNUSED(swapChain);
    }

    Result DeviceImpl::UpdateSwapChain(SwapChain* swapChain, const SwapChainDesc& desc)
    {
        HE_UNUSED(swapChain, desc);
        return Result::Success;
    }

    PresentTarget DeviceImpl::AcquirePresentTarget(SwapChain* swapChain)
    {
        HE_UNUSED(swapChain);
        return PresentTarget{};
    }

    bool DeviceImpl::IsFullscreen(SwapChain* swapChain)
    {
        HE_UNUSED(swapChain);
        return false;
    }

    Result DeviceImpl::SetFullscreen(SwapChain* swapChain, bool fullscreen)
    {
        HE_UNUSED(swapChain, fullscreen);
        return Result::Success;
    }

    Result DeviceImpl::CreateTexture(const TextureDesc& desc, Texture*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyTexture(Texture* texture)
    {
        HE_UNUSED(texture);
    }

    Result DeviceImpl::CreateTextureView(const TextureViewDesc& desc, TextureView*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyTextureView(TextureView* view)
    {
        HE_UNUSED(view);
    }

    Result DeviceImpl::CreateRWTextureView(const TextureViewDesc& desc, RWTextureView*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRWTextureView(RWTextureView* view)
    {
        HE_UNUSED(view);
    }

    Result DeviceImpl::CreateRenderTargetView(const TextureViewDesc& desc, RenderTargetView*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderTargetView(RenderTargetView* view)
    {
        HE_UNUSED(view);
    }

    Result DeviceImpl::CreateConstantBufferView(const ConstantBufferViewDesc& desc, ConstantBufferView*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyConstantBufferView(ConstantBufferView* view)
    {
        HE_UNUSED(view);
    }

    Result DeviceImpl::CreateVertexBufferFormat(const VertexBufferFormatDesc& desc, VertexBufferFormat*& out)
    {
        HE_UNUSED(desc);
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyVertexBufferFormat(VertexBufferFormat* vbf)
    {
        HE_UNUSED(vbf);
    }

    Result DeviceImpl::GetSwapChainFormats(void* nvh, uint32_t& count, SwapChainFormat* formats)
    {
        HE_UNUSED(nvh);
        count = 1;
        if (formats)
            formats[0] = { Format::RGBA8Unorm_sRGB, ColorSpace::sRGB };
        return Result::Success;
    }
}

#endif
