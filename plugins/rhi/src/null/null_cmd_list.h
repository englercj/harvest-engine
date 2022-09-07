// Copyright Chad Engler

#pragma once

#include "he/rhi/cmd_list.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    class CopyCmdListImpl final : public CopyCmdList
    {
    public:
        Result Begin(CmdAllocator* alloc) override;
        Result End() override;

        void BeginGroup(const char* msg) override;
        void EndGroup() override;
        void SetMarker(const char* msg) override;

        void Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region = nullptr) override;
        void Copy(const Texture* src, const Texture* dst, const TextureCopy* region = nullptr) override;

        void Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region) override;
        void Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region) override;
    };

    class ComputeCmdListImpl final : public ComputeCmdList
    {
    public:
        Result Begin(CmdAllocator* alloc) override;
        Result End() override;

        void BeginGroup(const char* msg) override;
        void EndGroup() override;
        void SetMarker(const char* msg) override;

        void Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region = nullptr) override;
        void Copy(const Texture* src, const Texture* dst, const TextureCopy* region = nullptr) override;

        void Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region) override;
        void Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region) override;

        void Clear(const RWBufferView* view, const Vec4f& values) override;
        void Clear(const RWBufferView* view, const Vec4u& values) override;
        void Clear(const RWTextureView* view, const Vec4f& values) override;
        void Clear(const RWTextureView* view, const Vec4u& values) override;

        void SetComputeRootSignature(const RootSignature* signature) override;
        void SetComputePipeline(const ComputePipeline* pipeline) override;
        void SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table) override;
        void SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset = 0) override;
        void SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset = 0) override;
        void SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset = 0) override;

        void Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ) override;

        void TransitionBarrier(const Texture* texture, TextureState oldState, TextureState newState, uint32_t subresource = AllSubresources) override;
        void TransitionBarrier(const Buffer* buffer, BufferState oldState, BufferState newState) override;

        void RWBarrier(const Buffer* buffer) override;
        void RWBarrier(const Texture* texture) override;
    };

    class RenderCmdListImpl final : public RenderCmdList
    {
    public:
        Result Begin(CmdAllocator* alloc) override;
        Result End() override;

        void BeginGroup(const char* msg) override;
        void EndGroup() override;
        void SetMarker(const char* msg) override;

        void Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region = nullptr) override;
        void Copy(const Texture* src, const Texture* dst, const TextureCopy* region = nullptr) override;

        void Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region) override;
        void Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region) override;

        void Clear(const RWBufferView* view, const Vec4f& values) override;
        void Clear(const RWBufferView* view, const Vec4u& values) override;
        void Clear(const RWTextureView* view, const Vec4f& values) override;
        void Clear(const RWTextureView* view, const Vec4u& values) override;
        void Clear(const RenderTargetView* rtv, const Vec4f& values) override;
        void Clear(const RenderTargetView* rtv, ClearFlag flags, float depthValue, uint8_t stencilValue) override;

        void SetComputeRootSignature(const RootSignature* signature) override;
        void SetComputePipeline(const ComputePipeline* pipeline) override;
        void SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table) override;
        void SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset = 0) override;
        void SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset = 0) override;
        void SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset = 0) override;

        void Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ) override;

        void TransitionBarrier(const Texture* texture, TextureState oldState, TextureState newState, uint32_t subresource = AllSubresources) override;
        void TransitionBarrier(const Buffer* buffer, BufferState oldState, BufferState newState) override;

        void RWBarrier(const Buffer* buffer) override;
        void RWBarrier(const Texture* texture) override;

        void BeginRenderPass(const RenderPassDesc& desc) override;
        void EndRenderPass() override;

        void Resolve(const RenderTargetView* srcView, const RenderTargetView* dstView) override;

        void SetRenderRootSignature(const RootSignature* rootSignature) override;
        void SetRenderPipeline(const RenderPipeline* pipeline) override;
        void SetRenderDescriptorTable(uint32_t slot, const DescriptorTable* table) override;
        void SetRenderConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset = 0) override;
        void SetRender32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset = 0) override;
        void SetRender32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset = 0) override;

        void SetIndexBuffer(const Buffer* buffer, uint32_t offset, uint32_t size, IndexType type) override;
        void SetVertexBuffer(uint32_t index, const VertexBufferFormat* vbf, const Buffer* buffer, uint32_t offset, uint32_t size) override;
        void SetVertexBuffers(uint32_t index, uint32_t count, const VertexBufferFormat* const* vbfs, const Buffer* const* buffers, const uint32_t* offsets, const uint32_t* sizes) override;

        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Vec2u& pos, const Vec2u& size) override;
        void SetStencilRef(uint8_t value) override;
        void SetBlendColor(const Vec4f& color) override;
        void SetDepthBounds(float min, float max) override;

        void Draw(const DrawDesc& desc) override;
        void DrawIndexed(const DrawIndexedDesc& desc) override;
    };
}

#endif
