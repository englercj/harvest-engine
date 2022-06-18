// Copyright Chad Engler

#pragma once

#include "bitmap_allocator.h"
#include "d3d12_common.h"

#include "he/core/vector.h"
#include "he/core/wstr.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_D3D12

namespace he::rhi::d3d12
{
    struct TextureImpl;

    class DescriptorPool
    {
    public:
        ~DescriptorPool() noexcept;

        HRESULT Create(Allocator& allocator, ID3D12Device* d3dDevice, const D3D12_DESCRIPTOR_HEAP_DESC& d3dHeapDesc);
        void Destroy(Allocator& allocator);

        void Alloc(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle = nullptr);
        void Free(uint32_t count, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

        ID3D12DescriptorHeap* GetHeap() const { return m_heap; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_heapType; }
        uint32_t GetStride() const { return m_stride; }

    private:
        ID3D12DescriptorHeap* m_heap{ nullptr };
        D3D12_DESCRIPTOR_HEAP_TYPE m_heapType{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

        uint32_t m_stride{ 0 };
        D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart{ 0 };
        BitmapAllocator m_bitAlloc{};
    };

    struct DisplayModeImpl final : DisplayMode
    {
        DisplayModeInfo info{};
    };

    struct DisplayImpl final : Display
    {
        explicit DisplayImpl(Allocator& allocator) noexcept : info(allocator), modes(allocator) {}

        DisplayInfo info;
        Vector<DisplayModeImpl> modes;
    };

    struct AdapterImpl final : Adapter
    {
        explicit AdapterImpl(Allocator& allocator) noexcept : displays(allocator) {}

        AdapterInfo info{};
        IDXGIAdapter1* dxgiAdapter1{ nullptr };
        Vector<DisplayImpl> displays;
    };

    struct BufferImpl final : Buffer
    {
        HeapType heapType{ HeapType::Default };
        BufferUsage usage{ BufferUsage::Upload };
        uint32_t size{ 0 };
        uint32_t stride{ 1 };

        ID3D12Resource* d3dResource{ nullptr };
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{ 0 };
        void* cpuAddress{ nullptr };
    };

    struct BufferViewImpl final : BufferView
    {
        const BufferImpl* buffer{ nullptr };
        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle{ 0 };
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{ 0 };
    };

    struct CmdAllocatorImpl final : CmdAllocator
    {
        ID3D12CommandAllocator* d3dCmdAllocator{ nullptr };
    };

    struct ComputePipelineImpl final : ComputePipeline
    {
        ID3D12PipelineState* d3dPipeline{ nullptr };
    };

    struct ConstantBufferViewImpl final : ConstantBufferView
    {
        // const BufferImpl* buffer{ nullptr };
        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle{ 0 };
    };

    struct CpuFenceImpl final : CpuFence
    {
        uint64_t value{ 1 };
        HANDLE event{ nullptr };
        ID3D12Fence* d3dFence{ nullptr };
    };

    struct DescriptorTableImpl final : DescriptorTable
    {
        explicit DescriptorTableImpl(Allocator& allocator) noexcept : ranges(allocator) {}

        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuStart{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuStart{ 0 };

        Vector<DescriptorRange> ranges;
    };

    struct GpuFenceImpl final : GpuFence
    {
        ID3D12Fence* d3dFence{ nullptr };
    };

    struct RenderPipelineImpl final : RenderPipeline
    {
        ID3D12PipelineState* d3dPipeline{ nullptr };
        D3D12_PRIMITIVE_TOPOLOGY topology{ D3D_PRIMITIVE_TOPOLOGY_UNDEFINED };
    };

    struct RenderTargetViewImpl final : RenderTargetView
    {
        const TextureImpl* texture{ nullptr };
        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle{ 0 };
        Format format{ Format::Invalid };
        uint32_t firstSubresource{ 0 };
    };

    struct RootSignatureImpl final : RootSignature
    {
        ID3D12RootSignature* d3dRootSignature{ nullptr };
    };

    struct RWBufferViewImpl final : RWBufferView
    {
        const BufferImpl* buffer{ nullptr };
        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuHandle{ 0 };
    };

    struct RWTextureViewImpl final : RWTextureView
    {
        const TextureImpl* texture{ nullptr };
        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuHandle{ 0 };
        uint32_t firstSubresource{ 0 };
    };

    struct SamplerImpl final : Sampler
    {
        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle{ 0 };
    };

    struct ShaderImpl final : Shader
    {
        ShaderStage stage{ ShaderStage::Pixel };
        D3D12_SHADER_BYTECODE byteCode{};
    };

    struct SwapChainImpl final : SwapChain
    {
        explicit SwapChainImpl(Allocator& allocator) noexcept : d3dResources(allocator), d3dRtvCpuHandles(allocator) {}

        IDXGISwapChain3* dxgiSwapChain3{ nullptr };
        HANDLE swapEvent{ nullptr };

        SwapChainFormat format{};
        uint32_t bufferIndex{ 0 };
        uint32_t syncInterval{ 0 };
        bool colorSpaceEnabled{ false };

        TextureImpl* texture{ nullptr };
        RenderTargetViewImpl* renderTargetView{ nullptr };

        Vector<ID3D12Resource*> d3dResources;
        Vector<D3D12_CPU_DESCRIPTOR_HANDLE> d3dRtvCpuHandles;
    };

    struct TextureImpl final : Texture
    {
        Format format{ Format::Invalid };
        uint32_t mipCount{ 0 };
        uint32_t layerCount{ 0 };
        TextureType type{ TextureType::_2D };

        ID3D12Resource* d3dResource{ nullptr };
    };

    struct TextureViewImpl final : TextureView
    {
        const TextureImpl* texture{ nullptr };
        D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuHandle{ 0 };
        uint32_t firstSubresource{ 0 };
    };

    struct VertexBufferFormatImpl final : VertexBufferFormat
    {
        explicit VertexBufferFormatImpl(Allocator& allocator) noexcept : attributes(allocator) {}

        uint32_t stride{ 0 };
        StepRate stepRate{ StepRate::PerVertex };
        Vector<VertexAttributeDesc> attributes;
    };
}

#endif
