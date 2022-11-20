// Copyright Chad Engler

#include "null_cmd_list.h"

#include "null_device.h"
#include "null_cmd_list.h"
#include "null_instance.h"

#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    // --------------------------------------------------------------------------------------------
    // Copy Command List

    Result CopyCmdListImpl::Begin(CmdAllocator* alloc)
    {
        HE_UNUSED(alloc);
        return Result::Success;
    }

    Result CopyCmdListImpl::End()
    {
        return Result::Success;
    }

    void CopyCmdListImpl::BeginGroup(const char* msg)
    {
        HE_UNUSED(msg);
    }

    void CopyCmdListImpl::EndGroup()
    {

    }

    void CopyCmdListImpl::SetMarker(const char* msg)
    {
        HE_UNUSED(msg);
    }

    void CopyCmdListImpl::Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region)
    {
        HE_UNUSED(src, dst, region);
    }

    void CopyCmdListImpl::Copy(const Texture* src, const Texture* dst, const TextureCopy* region)
    {
        HE_UNUSED(src, dst, region);
    }

    void CopyCmdListImpl::Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region)
    {
        HE_UNUSED(src, dst, region);
    }

    void CopyCmdListImpl::Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region)
    {
        HE_UNUSED(src, dst, region);
    }


    // --------------------------------------------------------------------------------------------
    // Compute Command List

    Result ComputeCmdListImpl::Begin(CmdAllocator* alloc)
    {
        HE_UNUSED(alloc);
        return Result::Success;
    }

    Result ComputeCmdListImpl::End()
    {
        return Result::Success;
    }

    void ComputeCmdListImpl::BeginGroup(const char* msg)
    {
        HE_UNUSED(msg);
    }

    void ComputeCmdListImpl::EndGroup()
    {

    }

    void ComputeCmdListImpl::SetMarker(const char* msg)
    {
        HE_UNUSED(msg);
    }

    void ComputeCmdListImpl::Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region)
    {
        HE_UNUSED(src, dst, region);
    }

    void ComputeCmdListImpl::Copy(const Texture* src, const Texture* dst, const TextureCopy* region)
    {
        HE_UNUSED(src, dst, region);
    }

    void ComputeCmdListImpl::Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region)
    {
        HE_UNUSED(src, dst, region);
    }

    void ComputeCmdListImpl::Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region)
    {
        HE_UNUSED(src, dst, region);
    }

    void ComputeCmdListImpl::Clear(const RWBufferView* view, const Vec4f& values)
    {
        HE_UNUSED(view, values);
    }

    void ComputeCmdListImpl::Clear(const RWBufferView* view, const Vec4u& values)
    {
        HE_UNUSED(view, values);
    }

    void ComputeCmdListImpl::Clear(const RWTextureView* view, const Vec4f& values)
    {
        HE_UNUSED(view, values);
    }

    void ComputeCmdListImpl::Clear(const RWTextureView* view, const Vec4u& values)
    {
        HE_UNUSED(view, values);
    }

    void ComputeCmdListImpl::SetComputeRootSignature(const RootSignature* signature)
    {
        HE_UNUSED(signature);
    }

    void ComputeCmdListImpl::SetComputePipeline(const ComputePipeline* pipeline)
    {
        HE_UNUSED(pipeline);
    }

    void ComputeCmdListImpl::SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table)
    {
        HE_UNUSED(slot, table);
    }

    void ComputeCmdListImpl::SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset)
    {
        HE_UNUSED(slot, buffer, offset);
    }

    void ComputeCmdListImpl::SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset)
    {
        HE_UNUSED(slot, value, offset);
    }

    void ComputeCmdListImpl::SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset)
    {
        HE_UNUSED(slot, data, count, offset);
    }

    void ComputeCmdListImpl::Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ)
    {
        HE_UNUSED(countX, countY, countZ);
    }

    void ComputeCmdListImpl::TransitionBarrier(const Texture* texture, TextureState oldState, TextureState newState, uint32_t subresource)
    {
        HE_UNUSED(texture, oldState, newState, subresource);
    }

    void ComputeCmdListImpl::TransitionBarrier(const Buffer* buffer, BufferState oldState, BufferState newState)
    {
        HE_UNUSED(buffer, oldState, newState);
    }

    void ComputeCmdListImpl::RWBarrier(const Buffer* buffer)
    {
        HE_UNUSED(buffer);
    }

    void ComputeCmdListImpl::RWBarrier(const Texture* texture)
    {
        HE_UNUSED(texture);
    }


    // --------------------------------------------------------------------------------------------
    // Render Command List

    Result RenderCmdListImpl::Begin(CmdAllocator* alloc)
    {
        HE_UNUSED(alloc);
        return Result::Success;
    }

    Result RenderCmdListImpl::End()
    {
        return Result::Success;
    }

    void RenderCmdListImpl::BeginGroup(const char* msg)
    {
        HE_UNUSED(msg);
    }

    void RenderCmdListImpl::EndGroup()
    {

    }

    void RenderCmdListImpl::SetMarker(const char* msg)
    {
        HE_UNUSED(msg);
    }

    void RenderCmdListImpl::Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region)
    {
        HE_UNUSED(src, dst, region);
    }

    void RenderCmdListImpl::Copy(const Texture* src, const Texture* dst, const TextureCopy* region)
    {
        HE_UNUSED(src, dst, region);
    }

    void RenderCmdListImpl::Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region)
    {
        HE_UNUSED(src, dst, region);
    }

    void RenderCmdListImpl::Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region)
    {
        HE_UNUSED(src, dst, region);
    }

    void RenderCmdListImpl::Clear(const RWBufferView* view, const Vec4f& values)
    {
        HE_UNUSED(view, values);
    }

    void RenderCmdListImpl::Clear(const RWBufferView* view, const Vec4u& values)
    {
        HE_UNUSED(view, values);
    }

    void RenderCmdListImpl::Clear(const RWTextureView* view, const Vec4f& values)
    {
        HE_UNUSED(view, values);
    }

    void RenderCmdListImpl::Clear(const RWTextureView* view, const Vec4u& values)
    {
        HE_UNUSED(view, values);
    }

    void RenderCmdListImpl::Clear(const RenderTargetView* rtv, const Vec4f& values)
    {
        HE_UNUSED(rtv, values);
    }

    void RenderCmdListImpl::Clear(const RenderTargetView* rtv, ClearFlag flags, float depthValue, uint8_t stencilValue)
    {
        HE_UNUSED(rtv, flags, depthValue, stencilValue);
    }

    void RenderCmdListImpl::SetComputeRootSignature(const RootSignature* signature)
    {
        HE_UNUSED(signature);
    }

    void RenderCmdListImpl::SetComputePipeline(const ComputePipeline* pipeline)
    {
        HE_UNUSED(pipeline);
    }

    void RenderCmdListImpl::SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table)
    {
        HE_UNUSED(slot, table);
    }

    void RenderCmdListImpl::SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset)
    {
        HE_UNUSED(slot, buffer, offset);
    }

    void RenderCmdListImpl::SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset)
    {
        HE_UNUSED(slot, value, offset);
    }

    void RenderCmdListImpl::SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset)
    {
        HE_UNUSED(slot, data, count, offset);
    }

    void RenderCmdListImpl::Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ)
    {
        HE_UNUSED(countX, countY, countZ);
    }

    void RenderCmdListImpl::TransitionBarrier(const Texture* texture, TextureState oldState, TextureState newState, uint32_t subresource)
    {
        HE_UNUSED(texture, oldState, newState, subresource);
    }

    void RenderCmdListImpl::TransitionBarrier(const Buffer* buffer, BufferState oldState, BufferState newState)
    {
        HE_UNUSED(buffer, oldState, newState);
    }

    void RenderCmdListImpl::RWBarrier(const Buffer* buffer)
    {
        HE_UNUSED(buffer);
    }

    void RenderCmdListImpl::RWBarrier(const Texture* texture)
    {
        HE_UNUSED(texture);
    }

    void RenderCmdListImpl::BeginRenderPass(const RenderPassDesc& desc)
    {
        HE_UNUSED(desc);
    }

    void RenderCmdListImpl::EndRenderPass()
    {

    }

    void RenderCmdListImpl::Resolve(const RenderTargetView* srcView, const RenderTargetView* dstView)
    {
        HE_UNUSED(srcView, dstView);
    }

    void RenderCmdListImpl::SetRenderRootSignature(const RootSignature* rootSignature)
    {
        HE_UNUSED(rootSignature);
    }

    void RenderCmdListImpl::SetRenderPipeline(const RenderPipeline* pipeline)
    {
        HE_UNUSED(pipeline);
    }

    void RenderCmdListImpl::SetRenderDescriptorTable(uint32_t slot, const DescriptorTable* table)
    {
        HE_UNUSED(slot, table);
    }

    void RenderCmdListImpl::SetRenderConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset)
    {
        HE_UNUSED(slot, buffer, offset);
    }

    void RenderCmdListImpl::SetRender32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset)
    {
        HE_UNUSED(slot, value, offset);
    }

    void RenderCmdListImpl::SetRender32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset)
    {
        HE_UNUSED(slot, data, count, offset);
    }

    void RenderCmdListImpl::SetIndexBuffer(const Buffer* buffer, uint32_t offset, uint32_t size, IndexType type)
    {
        HE_UNUSED(buffer, offset, size, type);
    }

    void RenderCmdListImpl::SetVertexBuffer(uint32_t index, const VertexBufferFormat* vbf, const Buffer* buffer, uint32_t offset, uint32_t size)
    {
        HE_UNUSED(index, vbf, buffer, offset, size);
    }

    void RenderCmdListImpl::SetVertexBuffers(uint32_t index, uint32_t count, const VertexBufferFormat* const* vbfs, const Buffer* const* buffers, const uint32_t* offsets, const uint32_t* sizes)
    {
        HE_UNUSED(index, count, vbfs, buffers, offsets, sizes);
    }

    void RenderCmdListImpl::SetViewport(const Viewport& viewport)
    {
        HE_UNUSED(viewport);
    }

    void RenderCmdListImpl::SetScissor(const Vec2u& pos, const Vec2u& size)
    {
        HE_UNUSED(pos, size);
    }

    void RenderCmdListImpl::SetStencilRef(uint8_t value)
    {
        HE_UNUSED(value);
    }

    void RenderCmdListImpl::SetBlendColor(const Vec4f& color)
    {
        HE_UNUSED(color);
    }

    void RenderCmdListImpl::SetDepthBounds(float min, float max)
    {
        HE_UNUSED(min, max);
    }

    void RenderCmdListImpl::Draw(const DrawDesc& desc)
    {
        HE_UNUSED(desc);
    }

    void RenderCmdListImpl::DrawIndexed(const DrawIndexedDesc& desc)
    {
        HE_UNUSED(desc);
    }
}

#endif
