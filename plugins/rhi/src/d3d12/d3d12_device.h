// Copyright Chad Engler

#pragma once

#include "d3d12_cmd_queue.h"
#include "d3d12_common.h"
#include "d3d12_resources.h"

#include "he/rhi/config.h"
#include "he/rhi/device.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    class InstanceImpl;
    struct AdapterImpl;

    class DeviceImpl final : public Device
    {
    public:
        ~DeviceImpl() noexcept;

        Result Initialize(InstanceImpl* instance, ID3D12Device* device, const AdapterImpl* adapter, const DeviceDesc& desc);
        Result MakeResult(HRESULT hr);

        Result CreateBuffer(const BufferDesc& desc, Buffer*& out) override;
        void DestroyBuffer(Buffer* buffer) override;

        Result CreateBufferView(const BufferViewDesc& desc, BufferView*& out) override;
        void DestroyBufferView(BufferView* view) override;

        Result CreateRWBufferView(const BufferViewDesc& desc, RWBufferView*& out) override;
        void DestroyRWBufferView(RWBufferView* view) override;

        void* Map(Buffer* buffer, uint32_t offset, uint32_t size) override;
        void Unmap(Buffer* buffer) override;

        Result CreateCopyCmdList(const CmdListDesc& desc, CopyCmdList*& out) override;
        void DestroyCopyCmdList(CopyCmdList* cmdList) override;

        Result CreateComputeCmdList(const CmdListDesc& desc, ComputeCmdList*& out) override;
        void DestroyComputeCmdList(ComputeCmdList* cmdList) override;

        Result CreateRenderCmdList(const CmdListDesc& desc, RenderCmdList*& out) override;
        void DestroyRenderCmdList(RenderCmdList* cmdList) override;

        Result CreateCmdAllocator(const CmdAllocatorDesc& desc, CmdAllocator*& out) override;
        void DestroyCmdAllocator(CmdAllocator* alloc) override;
        Result ResetCmdAllocator(CmdAllocator* alloc) override;

        CopyCmdQueue& GetCopyCmdQueue() override { return m_copyCmdQueue; }
        ComputeCmdQueue& GetComputeCmdQueue() override { return m_computeCmdQueue; }
        RenderCmdQueue& GetRenderCmdQueue() override { return m_renderCmdQueue; }

        Result CreateDescriptorTable(const DescriptorTableDesc& desc, DescriptorTable*& out) override;
        void DestroyDescriptorTable(DescriptorTable* table) override;

        void SetBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const BufferView* const* views) override;
        void SetTextureViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const TextureView* const* views) override;
        void SetRWBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWBufferView* const* views) override;
        void SetRWTextureViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWTextureView* const* views) override;
        void SetConstantBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const ConstantBufferView* const* views) override;
        void SetSamplers(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const Sampler* const* samplers) override;

        Result CreateCpuFence(const CpuFenceDesc& desc, CpuFence*& out) override;
        void DestroyCpuFence(CpuFence* fence) override;

        Result CreateGpuFence(const GpuFenceDesc& desc, GpuFence*& out) override;
        void DestroyGpuFence(GpuFence* fence) override;

        bool WaitForFence(const CpuFence* fence, uint32_t timeoutMs) override;
        bool IsFenceSignaled(const CpuFence* fence) override;
        uint64_t GetFenceValue(const GpuFence* fence) override;

        Result CreateComputePipeline(const ComputePipelineDesc& desc, ComputePipeline*& out) override;
        void DestroyComputePipeline(ComputePipeline* pipeline) override;

        Result CreateRenderPipeline(const RenderPipelineDesc& desc, RenderPipeline*& out) override;
        void DestroyRenderPipeline(RenderPipeline* pipeline) override;

        Result CreateRootSignature(const RootSignatureDesc& desc, RootSignature*& out) override;
        void DestroyRootSignature(RootSignature* signature) override;

        Result CreateSampler(const SamplerDesc& desc, Sampler*& out) override;
        void DestroySampler(Sampler* sampler) override;

        Result CreateShader(const ShaderDesc& desc, Shader*& out) override;
        void DestroyShader(Shader* shader) override;

        Result CreateSwapChain(const SwapChainDesc& desc, SwapChain*& out) override;
        void DestroySwapChain(SwapChain* swapChain) override;

        Result UpdateSwapChain(SwapChain* swapChain, const SwapChainDesc& desc) override;

        PresentTarget AcquirePresentTarget(SwapChain* swapChain) override;
        bool IsFullscreen(SwapChain* swapChain) override;
        Result SetFullscreen(SwapChain* swapChain, bool fullscreen) override;

        Result CreateTexture(const TextureDesc& desc, Texture*& out) override;
        void DestroyTexture(Texture* texture) override;

        Result CreateTextureView(const TextureViewDesc& desc, TextureView*& out) override;
        void DestroyTextureView(TextureView* view) override;

        Result CreateRWTextureView(const TextureViewDesc& desc, RWTextureView*& out) override;
        void DestroyRWTextureView(RWTextureView* view) override;

        Result CreateRenderTargetView(const TextureViewDesc& desc, RenderTargetView*& out) override;
        void DestroyRenderTargetView(RenderTargetView* view) override;

        Result CreateConstantBufferView(const ConstantBufferViewDesc& desc, ConstantBufferView*& out) override;
        void DestroyConstantBufferView(ConstantBufferView* view) override;

        Result CreateVertexBufferFormat(const VertexBufferFormatDesc& desc, VertexBufferFormat*& out) override;
        void DestroyVertexBufferFormat(VertexBufferFormat* vbf) override;

        const DeviceInfo& GetDeviceInfo() override { return m_info; }
        Result GetSwapChainFormats(void* nvh, SwapChainFormat* formats, uint32_t& count) override;

    private:
        HRESULT CreateSwapChainResources(SwapChain* swapChain);
        void SetColorSpace(SwapChain* swapChain);

        template <typename T, typename U>
        void SetDescriptorTableViews(
            DescriptorTable* table,
            uint32_t rangeIndex,
            uint32_t descIndex,
            uint32_t count,
            const U* const* views);

    public:
        InstanceImpl* m_instance{ nullptr };
        ID3D12Device* m_d3dDevice{ nullptr };
        IDXGIAdapter1* m_dxgiAdapter1{ nullptr };
        const AdapterImpl* m_adapter{ nullptr };

        DeviceInfo m_info{};

        CopyCmdQueueImpl m_copyCmdQueue{};
        ComputeCmdQueueImpl m_computeCmdQueue{};
        RenderCmdQueueImpl m_renderCmdQueue{};

        DescriptorPool m_cpuGeneralPool{};
        DescriptorPool m_cpuSamplerPool{};
        DescriptorPool m_cpuRtvPool{};
        DescriptorPool m_cpuDsvPool{};
        DescriptorPool m_gpuGeneralPool{};
        DescriptorPool m_gpuSamplerPool{};

        D3D12_CPU_DESCRIPTOR_HANDLE m_nullTextureSRV[static_cast<uint32_t>(TextureType::_Count)]{};
        D3D12_CPU_DESCRIPTOR_HANDLE m_nullTextureUAV[static_cast<uint32_t>(TextureType::_Count)]{};
        D3D12_CPU_DESCRIPTOR_HANDLE m_nullBufferSRV{ 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE m_nullBufferUAV{ 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE m_nullBufferCBV{ 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE m_nullSampler{ 0 };
    };
}

#endif
