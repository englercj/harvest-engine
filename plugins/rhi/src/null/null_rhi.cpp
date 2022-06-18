// Copyright Chad Engler

#include "rhi_internal.h"

#include "he/core/assert.h"
#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/config.h"
#include "he/rhi/device.h"
#include "he/rhi/instance.h"
#include "he/rhi/types.h"
#include "he/rhi/utils.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    class InstanceImpl;

    // --------------------------------------------------------------------------------------------
    class CopyCmdListImpl final : public virtual CopyCmdList
    {
    public:
        void Begin(CmdAllocator* alloc) override { HE_UNUSED(alloc); }
        Result End() override { return Result::Success; }

        void BeginGroup(const char* msg) override { HE_UNUSED(msg); }
        void EndGroup() override {}
        void SetMarker(const char* msg) override { HE_UNUSED(msg); }

        void Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region = nullptr) override { HE_UNUSED(src, dst, region); }
        void Copy(const Texture* src, const Texture* dst, const TextureCopy* region = nullptr) override { HE_UNUSED(src, dst, region); }

        void Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region) override { HE_UNUSED(src, dst, region); }
        void Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region) override { HE_UNUSED(src, dst, region); }
    };

    class ComputeCmdListImpl final : public ComputeCmdList
    {
    public:
        void Begin(CmdAllocator* alloc) override { HE_UNUSED(alloc); }
        Result End() override { return Result::Success; }

        void BeginGroup(const char* msg) override { HE_UNUSED(msg); }
        void EndGroup() override {}
        void SetMarker(const char* msg) override { HE_UNUSED(msg); }

        void Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region = nullptr) override { HE_UNUSED(src, dst, region); }
        void Copy(const Texture* src, const Texture* dst, const TextureCopy* region = nullptr) override { HE_UNUSED(src, dst, region); }

        void Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region) override { HE_UNUSED(src, dst, region); }
        void Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region) override { HE_UNUSED(src, dst, region); }

        void Clear(const RWBufferView* view, const Vec4f& values) override { HE_UNUSED(view, values); }
        void Clear(const RWBufferView* view, const Vec4u& values) override { HE_UNUSED(view, values); }
        void Clear(const RWTextureView* view, const Vec4f& values) override { HE_UNUSED(view, values); }
        void Clear(const RWTextureView* view, const Vec4u& values) override { HE_UNUSED(view, values); }

        void SetComputeRootSignature(const RootSignature* signature) override { HE_UNUSED(signature); }
        void SetComputePipeline(const ComputePipeline* pipeline) override { HE_UNUSED(pipeline); }
        void SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table) override { HE_UNUSED(slot, table); }
        void SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset = 0) override { HE_UNUSED(slot, buffer, offset); }
        void SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset = 0) override { HE_UNUSED(slot, value, offset); }
        void SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset = 0) override { HE_UNUSED(slot, data, count, offset); }

        void Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ) override { HE_UNUSED(countX, countY, countZ); }

        void TransitionBarrier(const Texture* texture, TextureState oldState, TextureState newState, uint32_t subresource = AllSubresources) override { HE_UNUSED(texture, oldState, newState, subresource); }
        void TransitionBarrier(const Buffer* buffer, BufferState oldState, BufferState newState) override { HE_UNUSED(buffer, oldState, newState); }

        void RWBarrier(const Buffer* buffer) override { HE_UNUSED(buffer); }
        void RWBarrier(const Texture* texture) override { HE_UNUSED(texture); }
    };

    class RenderCmdListImpl final : public RenderCmdList
    {
    public:
        void Begin(CmdAllocator* alloc) override { HE_UNUSED(alloc); }
        Result End() override { return Result::Success; }

        void BeginGroup(const char* msg) override { HE_UNUSED(msg); }
        void EndGroup() override {}
        void SetMarker(const char* msg) override { HE_UNUSED(msg); }

        void Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region = nullptr) override { HE_UNUSED(src, dst, region); }
        void Copy(const Texture* src, const Texture* dst, const TextureCopy* region = nullptr) override { HE_UNUSED(src, dst, region); }

        void Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region) override { HE_UNUSED(src, dst, region); }
        void Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region) override { HE_UNUSED(src, dst, region); }

        void Clear(const RWBufferView* view, const Vec4f& values) override { HE_UNUSED(view, values); }
        void Clear(const RWBufferView* view, const Vec4u& values) override { HE_UNUSED(view, values); }
        void Clear(const RWTextureView* view, const Vec4f& values) override { HE_UNUSED(view, values); }
        void Clear(const RWTextureView* view, const Vec4u& values) override { HE_UNUSED(view, values); }
        void Clear(const RenderTargetView* rtv, const Vec4f& values) override { HE_UNUSED(rtv, values); }
        void Clear(const RenderTargetView* rtv, ClearFlags flags, float depthValue, uint8_t stencilValue) override { HE_UNUSED(rtv, flags, depthValue, stencilValue); }

        void SetComputeRootSignature(const RootSignature* signature) override { HE_UNUSED(signature); }
        void SetComputePipeline(const ComputePipeline* pipeline) override { HE_UNUSED(pipeline); }
        void SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table) override { HE_UNUSED(slot, table); }
        void SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset = 0) override { HE_UNUSED(slot, buffer, offset); }
        void SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset = 0) override { HE_UNUSED(slot, value, offset); }
        void SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset = 0) override { HE_UNUSED(slot, data, count, offset); }

        void Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ) override { HE_UNUSED(countX, countY, countZ); }

        void TransitionBarrier(const Texture* texture, TextureState oldState, TextureState newState, uint32_t subresource = AllSubresources) override { HE_UNUSED(texture, oldState, newState, subresource); }
        void TransitionBarrier(const Buffer* buffer, BufferState oldState, BufferState newState) override { HE_UNUSED(buffer, oldState, newState); }

        void RWBarrier(const Buffer* buffer) override { HE_UNUSED(buffer); }
        void RWBarrier(const Texture* texture) override { HE_UNUSED(texture); }

        void BeginRenderPass(const RenderPassDesc& desc) override { HE_UNUSED(desc); }
        void EndRenderPass() override {}

        void Resolve(const RenderTargetView* srcView, const RenderTargetView* dstView) override { HE_UNUSED(srcView, dstView); }

        void SetRenderRootSignature(const RootSignature* rootSignature) override { HE_UNUSED(rootSignature); }
        void SetRenderPipeline(const RenderPipeline* pipeline) override { HE_UNUSED(pipeline); }
        void SetRenderDescriptorTable(uint32_t slot, const DescriptorTable* table) override { HE_UNUSED(slot, table); }
        void SetRenderConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset = 0) override { HE_UNUSED(slot, buffer, offset); }
        void SetRender32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset = 0) override { HE_UNUSED(slot, value, offset); }
        void SetRender32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset = 0) override { HE_UNUSED(slot, data, count, offset); }

        void SetIndexBuffer(const Buffer* buffer, uint32_t offset, uint32_t size, IndexType type) override { HE_UNUSED(buffer, offset, size, type); }
        void SetVertexBuffer(uint32_t index, const VertexBufferFormat* vbf, const Buffer* buffer, uint32_t offset, uint32_t size) override { HE_UNUSED(index, vbf, buffer, offset, size); }
        void SetVertexBuffers(uint32_t index, uint32_t count, const VertexBufferFormat* const* vbfs, const Buffer* const* buffers, const uint32_t* offsets, const uint32_t* sizes) override { HE_UNUSED(index, count, vbfs, buffers, offsets, sizes); }

        void SetViewport(const Viewport& viewport) override { HE_UNUSED(viewport); }
        void SetScissor(const Vec2u& pos, const Vec2u& size) override { HE_UNUSED(pos, size); }
        void SetStencilRef(uint8_t value) override { HE_UNUSED(value); }
        void SetBlendColor(const Vec4f& color) override { HE_UNUSED(color); }
        void SetDepthBounds(float min, float max) override { HE_UNUSED(min, max); }

        void Draw(const DrawDesc& desc) override { HE_UNUSED(desc); }
        void DrawIndexed(const DrawIndexedDesc& desc) override { HE_UNUSED(desc); }
    };

    // --------------------------------------------------------------------------------------------
    class CopyCmdQueueImpl final : public CopyCmdQueue
    {
    public:
        void Signal(CpuFence* fence) override { HE_UNUSED(fence); }
        void Signal(GpuFence* fence, uint64_t value) override { HE_UNUSED(fence, value); }
        void Wait(GpuFence* fence, uint64_t value) override { HE_UNUSED(fence, value); }

        void WaitForFlush() override {}

        void Submit(CopyCmdList* cmdList) override { HE_UNUSED(cmdList); }
    };

    class ComputeCmdQueueImpl final : public ComputeCmdQueue, public CmdQueueImpl
    {
    public:
        void Signal(CpuFence* fence) override { HE_UNUSED(fence); }
        void Signal(GpuFence* fence, uint64_t value) override { HE_UNUSED(fence, value); }
        void Wait(GpuFence* fence, uint64_t value) override { HE_UNUSED(fence, value); }

        void WaitForFlush() override {}

        void Submit(ComputeCmdList* cmdList) override { HE_UNUSED(cmdList); }
    };

    class RenderCmdQueueImpl final : public RenderCmdQueue, public CmdQueueImpl
    {
    public:
        void Signal(CpuFence* fence) override { HE_UNUSED(fence); }
        void Signal(GpuFence* fence, uint64_t value) override { HE_UNUSED(fence, value); }
        void Wait(GpuFence* fence, uint64_t value) override { HE_UNUSED(fence, value); }

        void WaitForFlush() override {}

        void Submit(RenderCmdList* cmdList) override { HE_UNUSED(cmdList); }
        Result Present(SwapChain* swapChain) override { HE_UNUSED(swapChain); }
    };

    // --------------------------------------------------------------------------------------------
    class DeviceImpl final : public Device
    {
    public:
        explicit DeviceImpl(InstanceImpl* instance) noexcept : m_instance(instance) {}

        Result CreateBuffer(const BufferDesc& desc, Buffer*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyBuffer(Buffer* buffer) override { HE_UNUSED(buffer); }

        Result CreateBufferView(const BufferViewDesc& desc, BufferView*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyBufferView(BufferView* view) override { HE_UNUSED(view); }

        Result CreateRWBufferView(const BufferViewDesc& desc, RWBufferView*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyRWBufferView(RWBufferView* view) override { HE_UNUSED(view); }

        void* Map(Buffer* buffer, uint32_t offset, uint32_t size) override { HE_UNUSED(buffer, offset, size); }
        void Unmap(Buffer* buffer) override { HE_UNUSED(buffer); }

        Result CreateCopyCmdList(const CmdListDesc& desc, CopyCmdList*& out) override { HE_UNUSED(desc); out = HE_NEW(m_instance->m_allocator, CopyCmdListImpl); return Result::Success; }
        void DestroyCopyCmdList(CopyCmdList* cmdList) override { HE_DELETE(m_instance->m_allocator, cmdList); }

        Result CreateComputeCmdList(const CmdListDesc& desc, ComputeCmdList*& out) override { HE_UNUSED(desc); out = HE_NEW(m_instance->m_allocator, ComputeCmdListImpl); return Result::Success; }
        void DestroyComputeCmdList(ComputeCmdList* cmdList) override { HE_DELETE(m_instance->m_allocator, cmdList); }

        Result CreateRenderCmdList(const CmdListDesc& desc, RenderCmdList*& out) override { HE_UNUSED(desc); out = HE_NEW(m_instance->m_allocator, RenderCmdListImpl); return Result::Success; }
        void DestroyRenderCmdList(RenderCmdList* cmdList) override { HE_DELETE(m_instance->m_allocator, cmdList); }

        Result CreateCmdAllocator(const CmdAllocatorDesc& desc, CmdAllocator*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyCmdAllocator(CmdAllocator* alloc) override { HE_UNUSED(alloc); }
        Result ResetCmdAllocator(CmdAllocator* alloc) override { HE_UNUSED(alloc); }

        CopyCmdQueue& GetCopyCmdQueue() override { return m_copyCmdQueue; }
        ComputeCmdQueue& GetComputeCmdQueue() override { return m_computeCmdQueue; }
        RenderCmdQueue& GetRenderCmdQueue() override { return m_renderCmdQueue; }

        Result CreateDescriptorTable(const DescriptorTableDesc& desc, DescriptorTable*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyDescriptorTable(DescriptorTable* table) override { HE_UNUSED(table); }

        void SetBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const BufferView* const* views) override { HE_UNUSED(table, rangeIndex, descIndex, count, views); }
        void SetTextureViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const TextureView* const* views) override { HE_UNUSED(table, rangeIndex, descIndex, count, views); }
        void SetRWBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWBufferView* const* views) override { HE_UNUSED(table, rangeIndex, descIndex, count, views); }
        void SetRWTextureViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWTextureView* const* views) override { HE_UNUSED(table, rangeIndex, descIndex, count, views); }
        void SetConstantBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const ConstantBufferView* views) override { HE_UNUSED(table, rangeIndex, descIndex, count, views); }
        void SetSamplers(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const Sampler* const* samplers) override { HE_UNUSED(table, rangeIndex, descIndex, count, samplers); }

        Result CreateCpuFence(const CpuFenceDesc& desc, CpuFence*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyCpuFence(CpuFence* fence) override { HE_UNUSED(fence); }

        Result CreateGpuFence(const GpuFenceDesc& desc, GpuFence*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyGpuFence(GpuFence* fence) override { HE_UNUSED(fence); }

        bool WaitForFence(const CpuFence* fence, uint32_t timeoutMs) override;
        bool IsFenceSignaled(const CpuFence* fence) override { HE_UNUSED(fence); }
        uint64_t GetFenceValue(const GpuFence* fence) override { HE_UNUSED(fence); }

        Result CreateComputePipeline(const ComputePipelineDesc& desc, ComputePipeline*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyComputePipeline(ComputePipeline* pipeline) override { HE_UNUSED(pipeline); }

        Result CreateRenderPipeline(const RenderPipelineDesc& desc, RenderPipeline*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyRenderPipeline(RenderPipeline* pipeline) override { HE_UNUSED(pipeline); }

        Result CreateRootSignature(const RootSignatureDesc& desc, RootSignature*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyRootSignature(RootSignature* signature) override { HE_UNUSED(signature); }

        Result CreateSampler(const SamplerDesc& desc, Sampler*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroySampler(Sampler* sampler) override { HE_UNUSED(sampler); }

        Result CreateShader(const ShaderDesc& desc, Shader*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyShader(Shader* shader) override { HE_UNUSED(shader); }

        Result CreateSwapChain(const SwapChainDesc& desc, SwapChain*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroySwapChain(SwapChain* swapChain) override { HE_UNUSED(swapChain); }

        Result UpdateSwapChain(SwapChain* swapChain, const SwapChainDesc& desc) override { HE_UNUSED(swapChain, desc); }

        PresentTarget AcquirePresentTarget(SwapChain* swapChain) override { HE_UNUSED(swapChain); return PresentTarget{}; }
        bool IsFullscreen(SwapChain* swapChain) override { HE_UNUSED(swapChain); return false; }
        Result SetFullscreen(SwapChain* swapChain, bool fullscreen) override { HE_UNUSED(swapChain, fullscreen); return Result::Success; }

        Result CreateTexture(const TextureDesc& desc, Texture*& out) override{ HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyTexture(Texture* texture) override { HE_UNUSED(texture); }

        Result CreateTextureView(const TextureViewDesc& desc, TextureView*& out) override{ HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyTextureView(TextureView* view) override { HE_UNUSED(view); }

        Result CreateRWTextureView(const TextureViewDesc& desc, RWTextureView*& out) override{ HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyRWTextureView(RWTextureView* view) override { HE_UNUSED(view); }

        Result CreateRenderTargetView(const TextureViewDesc& desc, RenderTargetView*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyRenderTargetView(RenderTargetView* view) override { HE_UNUSED(view); }

        Result CreateConstantBufferView(const ConstantBufferViewDesc& desc, ConstantBufferView*& out) override { HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyConstantBufferView(ConstantBufferView* view) override { HE_UNUSED(view); }

        Result CreateVertexBufferFormat(const VertexBufferFormatDesc& desc, VertexBufferFormat*& out) override{ HE_UNUSED(desc); out = nullptr; return Result::Success; }
        void DestroyVertexBufferFormat(VertexBufferFormat* vbf) override { HE_UNUSED(vbf); }

        const DeviceInfo& GetInfo() override { return m_info; }

        Result GetSwapChainFormats(void* nvh, uint32_t& count, SwapChainFormat* formats) override
        {
            HE_UNUSED(nvh);
            count = 1;
            if (formats)
                formats[0] = { Format::RGBA8Unorm_sRGB, ColorSpace::BT709_G22 };
            return Result::Success;
        }

    private:
        InstanceImpl* m_instance{ nullptr };

        DeviceInfo m_info{};

        CopyCmdQueueImpl m_copyCmdQueue{};
        ComputeCmdQueueImpl m_computeCmdQueue{};
        RenderCmdQueueImpl m_renderCmdQueue{};
    };

    // --------------------------------------------------------------------------------------------
    class InstanceImpl final : public Instance
    {
    public:
        explicit InstanceImpl(Allocator& allocator) noexcept : m_allocator(allocator) {}

        Result Initialize(const InstanceDesc& desc) override
        {
            HE_ASSERT(desc.allocator == nullptr || desc.allocator == &m_allocator);
            return Result::Success;
        }

        Allocator& GetAllocator() override { return *m_allocator; }
        ApiResult GetApiResult(Result result) const override { HE_UNUSED(result); return ApiResult::Success; }

        void GetAdapters(uint32_t& count, const Adapter*& adapters) override { count = 1; adapters = &m_adapter; }
        void GetDisplays(const Adapter& adapter, uint32_t& count, const Display*& displays) override { HE_UNUSED(adapter); count = 1; displays = &m_display; }
        void GetDisplayModes(const Display& display, uint32_t& count, const DisplayMode*& modes) override { HE_UNUSED(display); count = 1; modes = &m_displayMode; }

        const AdapterInfo& GetAdapterInfo(const Adapter& adapter) override { static AdapterInfo s_info{}; HE_UNUSED(adapter); return s_info; }
        const DisplayInfo& GetDisplayInfo(const Display& display) override { static DisplayInfo s_info{}; HE_UNUSED(display); return s_info; }
        const DisplayModeInfo& GetDisplayModeInfo(const DisplayMode& mode) override { statis DisplayModeInfo s_info{}; HE_UNUSED(mode); return s_info; }

        Result CreateDevice(const DeviceDesc& desc, Device*& out) override { out = HE_NEW(*m_allocator, DeviceImpl, this); }
        void DestroyDevice(Device* device) override { HE_DELETE(*m_allocator, device); };

    public:
        Allocator* m_allocator{ nullptr };

        struct AdapterImpl : Adapter {} m_adapter;
        struct DisplayImpl : Display {} m_display;
        struct DisplayModeImpl : DisplayMode {} m_displayMode;
    };
}

namespace he::rhi
{
    template <> Result _CreateInstance<ApiBackend::Null>(Allocator& allocator, Instance*& instance)
    {
        HE_LOGF_INFO(he_rhi, "Initializing NULL RHI backend.");
        instance = allocator.New<null::InstanceImpl>(allocator);
        return Result::Success;
    }
}

#endif
