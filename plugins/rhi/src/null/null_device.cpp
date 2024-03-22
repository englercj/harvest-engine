// Copyright Chad Engler

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

    Result DeviceImpl::CreateBuffer([[maybe_unused]] const BufferDesc& desc, Buffer*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyBuffer([[maybe_unused]] Buffer* buffer)
    {
    }

    Result DeviceImpl::CreateBufferView([[maybe_unused]] const BufferViewDesc& desc, BufferView*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyBufferView([[maybe_unused]] BufferView* view)
    {
    }

    Result DeviceImpl::CreateRWBufferView([[maybe_unused]] const BufferViewDesc& desc, RWBufferView*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRWBufferView([[maybe_unused]] RWBufferView* view)
    {
    }

    void*DeviceImpl:: Map([[maybe_unused]] Buffer* buffer, [[maybe_unused]] uint32_t offset, [[maybe_unused]] uint32_t size)
    {
        return nullptr;
    }

    void DeviceImpl::Unmap([[maybe_unused]] Buffer* buffer)
    {
    }

    Result DeviceImpl::CreateCopyCmdList([[maybe_unused]] const CmdListDesc& desc, CopyCmdList*& out)
    {
        out = m_instance->GetAllocator().New<CopyCmdListImpl>();
        return Result::Success;
    }

    void DeviceImpl::DestroyCopyCmdList(CopyCmdList* cmdList)
    {
        m_instance->GetAllocator().Delete(cmdList);
    }

    Result DeviceImpl::CreateComputeCmdList([[maybe_unused]] const CmdListDesc& desc, ComputeCmdList*& out)
    {
        out = m_instance->GetAllocator().New<ComputeCmdListImpl>();
        return Result::Success;
    }

    void DeviceImpl::DestroyComputeCmdList(ComputeCmdList* cmdList)
    {
        m_instance->GetAllocator().Delete(cmdList);
    }

    Result DeviceImpl::CreateRenderCmdList([[maybe_unused]] const CmdListDesc& desc, RenderCmdList*& out)
    {
        out = m_instance->GetAllocator().New<RenderCmdListImpl>();
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderCmdList(RenderCmdList* cmdList)
    {
        m_instance->GetAllocator().Delete(cmdList);
    }

    Result DeviceImpl::CreateCmdAllocator([[maybe_unused]] const CmdAllocatorDesc& desc, CmdAllocator*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyCmdAllocator([[maybe_unused]] CmdAllocator* alloc)
    {
    }

    Result DeviceImpl::ResetCmdAllocator([[maybe_unused]] CmdAllocator* alloc)
    {
        return Result::Success;
    }

    Result DeviceImpl::CreateDescriptorTable([[maybe_unused]] const DescriptorTableDesc& desc, DescriptorTable*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyDescriptorTable([[maybe_unused]] DescriptorTable* table)
    {
    }

    void DeviceImpl::SetBufferViews([[maybe_unused]] DescriptorTable* table, [[maybe_unused]] uint32_t rangeIndex, [[maybe_unused]] uint32_t descIndex, [[maybe_unused]] uint32_t count, [[maybe_unused]] const BufferView* const* views)
    {
    }

    void DeviceImpl::SetTextureViews([[maybe_unused]] DescriptorTable* table, [[maybe_unused]] uint32_t rangeIndex, [[maybe_unused]] uint32_t descIndex, [[maybe_unused]] uint32_t count, [[maybe_unused]] const TextureView* const* views)
    {
    }

    void DeviceImpl::SetRWBufferViews([[maybe_unused]] DescriptorTable* table, [[maybe_unused]] uint32_t rangeIndex, [[maybe_unused]] uint32_t descIndex, [[maybe_unused]] uint32_t count, [[maybe_unused]] const RWBufferView* const* views)
    {
    }

    void DeviceImpl::SetRWTextureViews([[maybe_unused]] DescriptorTable* table, [[maybe_unused]] uint32_t rangeIndex, [[maybe_unused]] uint32_t descIndex, [[maybe_unused]] uint32_t count, [[maybe_unused]] const RWTextureView* const* views)
    {
    }

    void DeviceImpl::SetConstantBufferViews([[maybe_unused]] DescriptorTable* table, [[maybe_unused]] uint32_t rangeIndex, [[maybe_unused]] uint32_t descIndex, [[maybe_unused]] uint32_t count, [[maybe_unused]] const ConstantBufferView* const* views)
    {
    }

    void DeviceImpl::SetSamplers([[maybe_unused]] DescriptorTable* table, [[maybe_unused]] uint32_t rangeIndex, [[maybe_unused]] uint32_t descIndex, [[maybe_unused]] uint32_t count, [[maybe_unused]] const Sampler* const* samplers)
    {
    }

    Result DeviceImpl::CreateCpuFence([[maybe_unused]] const CpuFenceDesc& desc, CpuFence*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyCpuFence([[maybe_unused]] CpuFence* fence)
    {
    }

    Result DeviceImpl::CreateGpuFence([[maybe_unused]] const GpuFenceDesc& desc, GpuFence*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyGpuFence([[maybe_unused]] GpuFence* fence)
    {
    }

    bool DeviceImpl::WaitForFence([[maybe_unused]] const CpuFence* fence, [[maybe_unused]] uint32_t timeoutMs)
    {
        return true;
    }

    bool DeviceImpl::IsFenceSignaled([[maybe_unused]] const CpuFence* fence)
    {
        return true;
    }

    uint64_t DeviceImpl::GetFenceValue([[maybe_unused]] const GpuFence* fence)
    {
        return 0;
    }

    Result DeviceImpl::CreateComputePipeline([[maybe_unused]] const ComputePipelineDesc& desc, ComputePipeline*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyComputePipeline([[maybe_unused]] ComputePipeline* pipeline)
    {
    }

    Result DeviceImpl::CreateRenderPipeline([[maybe_unused]] const RenderPipelineDesc& desc, RenderPipeline*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderPipeline([[maybe_unused]] RenderPipeline* pipeline)
    {
    }

    Result DeviceImpl::CreateRootSignature([[maybe_unused]] const RootSignatureDesc& desc, RootSignature*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRootSignature([[maybe_unused]] RootSignature* signature)
    {
    }

    Result DeviceImpl::CreateSampler([[maybe_unused]] const SamplerDesc& desc, Sampler*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroySampler([[maybe_unused]] Sampler* sampler)
    {
    }

    Result DeviceImpl::CreateShader([[maybe_unused]] const ShaderDesc& desc, Shader*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyShader([[maybe_unused]] Shader* shader)
    {
    }

    Result DeviceImpl::CreateSwapChain([[maybe_unused]] const SwapChainDesc& desc, SwapChain*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroySwapChain([[maybe_unused]] SwapChain* swapChain)
    {
    }

    Result DeviceImpl::UpdateSwapChain([[maybe_unused]] SwapChain* swapChain, [[maybe_unused]] const SwapChainDesc& desc)
    {
        return Result::Success;
    }

    PresentTarget DeviceImpl::AcquirePresentTarget([[maybe_unused]] SwapChain* swapChain)
    {
        return PresentTarget{};
    }

    bool DeviceImpl::IsFullscreen([[maybe_unused]] SwapChain* swapChain)
    {
        return false;
    }

    Result DeviceImpl::SetFullscreen([[maybe_unused]] SwapChain* swapChain, [[maybe_unused]] bool fullscreen)
    {
        return Result::Success;
    }

    Result DeviceImpl::CreateTexture([[maybe_unused]] const TextureDesc& desc, Texture*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyTexture([[maybe_unused]] Texture* texture)
    {
    }

    Result DeviceImpl::CreateTextureView([[maybe_unused]] const TextureViewDesc& desc, TextureView*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyTextureView([[maybe_unused]] TextureView* view)
    {
    }

    Result DeviceImpl::CreateRWTextureView([[maybe_unused]] const TextureViewDesc& desc, RWTextureView*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRWTextureView([[maybe_unused]] RWTextureView* view)
    {
    }

    Result DeviceImpl::CreateRenderTargetView([[maybe_unused]] const TextureViewDesc& desc, RenderTargetView*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyRenderTargetView([[maybe_unused]] RenderTargetView* view)
    {
    }

    Result DeviceImpl::CreateConstantBufferView([[maybe_unused]] const ConstantBufferViewDesc& desc, ConstantBufferView*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyConstantBufferView([[maybe_unused]] ConstantBufferView* view)
    {
    }

    Result DeviceImpl::CreateVertexBufferFormat([[maybe_unused]] const VertexBufferFormatDesc& desc, VertexBufferFormat*& out)
    {
        out = nullptr;
        return Result::Success;
    }

    void DeviceImpl::DestroyVertexBufferFormat([[maybe_unused]] VertexBufferFormat* vbf)
    {
    }

    Result DeviceImpl::GetSwapChainFormats([[maybe_unused]] void* nvh, SwapChainFormat* formats, uint32_t& count)
    {
        if (formats && count >= 1)
            formats[0] = { Format::RGBA8Unorm, ColorSpace::sRGB };

        count = 1;
        return Result::Success;
    }
}

#endif
