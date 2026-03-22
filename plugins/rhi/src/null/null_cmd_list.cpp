// Copyright Chad Engler

#include "null_cmd_list.h"

#include "null_device.h"
#include "null_cmd_list.h"
#include "null_instance.h"
#include "null_resources.h"

#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/rhi/config.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_NULL

namespace he::rhi::null
{
    namespace
    {
        void WriteTimestampInternal(uint64_t& nextTimestamp, const TimestampQuerySet* querySet_, uint32_t index)
        {
            TimestampQuerySetImpl* querySet = static_cast<TimestampQuerySetImpl*>(const_cast<TimestampQuerySet*>(querySet_));
            HE_ASSERT(querySet);
            HE_ASSERT(index < querySet->timestamps.Size());
            nextTimestamp += 1000;
            querySet->timestamps[index] = nextTimestamp;
        }

        void ResolveTimestampsInternal(const TimestampQuerySet* querySet_, uint32_t firstIndex, uint32_t count, const Buffer* dst_, uint32_t dstOffset)
        {
            const TimestampQuerySetImpl* querySet = static_cast<const TimestampQuerySetImpl*>(querySet_);
            BufferImpl* dst = static_cast<BufferImpl*>(const_cast<Buffer*>(dst_));
            HE_ASSERT(querySet);
            HE_ASSERT(dst);
            HE_ASSERT((firstIndex + count) <= querySet->timestamps.Size());
            HE_ASSERT((dstOffset + (count * sizeof(uint64_t))) <= dst->size);
            MemCopy(dst->data.Data() + dstOffset, querySet->timestamps.Data() + firstIndex, count * sizeof(uint64_t));
        }
    }

    // --------------------------------------------------------------------------------------------
    // Copy Command List

    Result CopyCmdListImpl::Begin([[maybe_unused]] CmdAllocator* alloc)
    {
        return Result::Success;
    }

    Result CopyCmdListImpl::End()
    {
        return Result::Success;
    }

    void CopyCmdListImpl::BeginGroup([[maybe_unused]] const char* msg)
    {
    }

    void CopyCmdListImpl::EndGroup()
    {
    }

    void CopyCmdListImpl::SetMarker([[maybe_unused]] const char* msg)
    {
    }

    void CopyCmdListImpl::WriteTimestamp(const TimestampQuerySet* querySet, uint32_t index)
    {
        WriteTimestampInternal(m_nextTimestamp, querySet, index);
    }

    void CopyCmdListImpl::ResolveTimestamps(const TimestampQuerySet* querySet, uint32_t firstIndex, uint32_t count, const Buffer* dst, uint32_t dstOffset)
    {
        ResolveTimestampsInternal(querySet, firstIndex, count, dst, dstOffset);
    }

    void CopyCmdListImpl::Copy([[maybe_unused]] const Buffer* src, [[maybe_unused]] const Buffer* dst, [[maybe_unused]] const BufferCopy* region)
    {
    }

    void CopyCmdListImpl::Copy([[maybe_unused]] const Texture* src, [[maybe_unused]] const Texture* dst, [[maybe_unused]] const TextureCopy* region)
    {
    }

    void CopyCmdListImpl::Copy([[maybe_unused]] const Buffer* src, [[maybe_unused]] const Texture* dst, [[maybe_unused]] const BufferTextureCopy& region)
    {
    }

    void CopyCmdListImpl::Copy([[maybe_unused]] const Texture* src, [[maybe_unused]] const Buffer* dst, [[maybe_unused]] const BufferTextureCopy& region)
    {
    }

    // --------------------------------------------------------------------------------------------
    // Compute Command List

    Result ComputeCmdListImpl::Begin([[maybe_unused]] CmdAllocator* alloc)
    {
        return Result::Success;
    }

    Result ComputeCmdListImpl::End()
    {
        return Result::Success;
    }

    void ComputeCmdListImpl::BeginGroup([[maybe_unused]] const char* msg)
    {
    }

    void ComputeCmdListImpl::EndGroup()
    {
    }

    void ComputeCmdListImpl::SetMarker([[maybe_unused]] const char* msg)
    {
    }

    void ComputeCmdListImpl::WriteTimestamp(const TimestampQuerySet* querySet, uint32_t index)
    {
        WriteTimestampInternal(m_nextTimestamp, querySet, index);
    }

    void ComputeCmdListImpl::ResolveTimestamps(const TimestampQuerySet* querySet, uint32_t firstIndex, uint32_t count, const Buffer* dst, uint32_t dstOffset)
    {
        ResolveTimestampsInternal(querySet, firstIndex, count, dst, dstOffset);
    }

    void ComputeCmdListImpl::Copy([[maybe_unused]] const Buffer* src, [[maybe_unused]] const Buffer* dst, [[maybe_unused]] const BufferCopy* region)
    {
    }

    void ComputeCmdListImpl::Copy([[maybe_unused]] const Texture* src, [[maybe_unused]] const Texture* dst, [[maybe_unused]] const TextureCopy* region)
    {
    }

    void ComputeCmdListImpl::Copy([[maybe_unused]] const Buffer* src, [[maybe_unused]] const Texture* dst, [[maybe_unused]] const BufferTextureCopy& region)
    {
    }

    void ComputeCmdListImpl::Copy([[maybe_unused]] const Texture* src, [[maybe_unused]] const Buffer* dst, [[maybe_unused]] const BufferTextureCopy& region)
    {
    }

    void ComputeCmdListImpl::Clear([[maybe_unused]] const RWBufferView* view, [[maybe_unused]] const Vec4f& values)
    {
    }

    void ComputeCmdListImpl::Clear([[maybe_unused]] const RWBufferView* view, [[maybe_unused]] const Vec4u& values)
    {
    }

    void ComputeCmdListImpl::Clear([[maybe_unused]] const RWTextureView* view, [[maybe_unused]] const Vec4f& values)
    {
    }

    void ComputeCmdListImpl::Clear([[maybe_unused]] const RWTextureView* view, [[maybe_unused]] const Vec4u& values)
    {
    }

    void ComputeCmdListImpl::SetComputeRootSignature([[maybe_unused]] const RootSignature* signature)
    {
    }

    void ComputeCmdListImpl::SetComputePipeline([[maybe_unused]] const ComputePipeline* pipeline)
    {
    }

    void ComputeCmdListImpl::SetComputeDescriptorTable([[maybe_unused]] uint32_t slot, [[maybe_unused]] const DescriptorTable* table)
    {
    }

    void ComputeCmdListImpl::SetComputeConstantBuffer([[maybe_unused]] uint32_t slot, [[maybe_unused]] const Buffer* buffer, [[maybe_unused]] uint32_t offset)
    {
    }

    void ComputeCmdListImpl::SetCompute32BitConstantValue([[maybe_unused]] uint32_t slot, [[maybe_unused]] uint32_t value, [[maybe_unused]] uint32_t offset)
    {
    }

    void ComputeCmdListImpl::SetCompute32BitConstantValues([[maybe_unused]] uint32_t slot, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t count, [[maybe_unused]] uint32_t offset)
    {
    }

    void ComputeCmdListImpl::Dispatch([[maybe_unused]] uint32_t countX, [[maybe_unused]] uint32_t countY, [[maybe_unused]] uint32_t countZ)
    {
    }

    void ComputeCmdListImpl::TransitionBarrier([[maybe_unused]] const Texture* texture, [[maybe_unused]] TextureState oldState, [[maybe_unused]] TextureState newState, [[maybe_unused]] uint32_t subresource)
    {
    }

    void ComputeCmdListImpl::TransitionBarrier([[maybe_unused]] const Buffer* buffer, [[maybe_unused]] BufferState oldState, [[maybe_unused]] BufferState newState)
    {
    }

    void ComputeCmdListImpl::RWBarrier([[maybe_unused]] const Buffer* buffer)
    {
    }

    void ComputeCmdListImpl::RWBarrier([[maybe_unused]] const Texture* texture)
    {
    }

    // --------------------------------------------------------------------------------------------
    // Render Command List

    Result RenderCmdListImpl::Begin([[maybe_unused]] CmdAllocator* alloc)
    {
        return Result::Success;
    }

    Result RenderCmdListImpl::End()
    {
        return Result::Success;
    }

    void RenderCmdListImpl::BeginGroup([[maybe_unused]] const char* msg)
    {
    }

    void RenderCmdListImpl::EndGroup()
    {
    }

    void RenderCmdListImpl::SetMarker([[maybe_unused]] const char* msg)
    {
    }

    void RenderCmdListImpl::WriteTimestamp(const TimestampQuerySet* querySet, uint32_t index)
    {
        WriteTimestampInternal(m_nextTimestamp, querySet, index);
    }

    void RenderCmdListImpl::ResolveTimestamps(const TimestampQuerySet* querySet, uint32_t firstIndex, uint32_t count, const Buffer* dst, uint32_t dstOffset)
    {
        ResolveTimestampsInternal(querySet, firstIndex, count, dst, dstOffset);
    }

    void RenderCmdListImpl::Copy([[maybe_unused]] const Buffer* src, [[maybe_unused]] const Buffer* dst, [[maybe_unused]] const BufferCopy* region)
    {
    }

    void RenderCmdListImpl::Copy([[maybe_unused]] const Texture* src, [[maybe_unused]] const Texture* dst, [[maybe_unused]] const TextureCopy* region)
    {
    }

    void RenderCmdListImpl::Copy([[maybe_unused]] const Buffer* src, [[maybe_unused]] const Texture* dst, [[maybe_unused]] const BufferTextureCopy& region)
    {
    }

    void RenderCmdListImpl::Copy([[maybe_unused]] const Texture* src, [[maybe_unused]] const Buffer* dst, [[maybe_unused]] const BufferTextureCopy& region)
    {
    }

    void RenderCmdListImpl::Clear([[maybe_unused]] const RWBufferView* view, [[maybe_unused]] const Vec4f& values)
    {
    }

    void RenderCmdListImpl::Clear([[maybe_unused]] const RWBufferView* view, [[maybe_unused]] const Vec4u& values)
    {
    }

    void RenderCmdListImpl::Clear([[maybe_unused]] const RWTextureView* view, [[maybe_unused]] const Vec4f& values)
    {
    }

    void RenderCmdListImpl::Clear([[maybe_unused]] const RWTextureView* view, [[maybe_unused]] const Vec4u& values)
    {
    }

    void RenderCmdListImpl::Clear([[maybe_unused]] const RenderTargetView* rtv, [[maybe_unused]] const Vec4f& values)
    {
    }

    void RenderCmdListImpl::Clear([[maybe_unused]] const RenderTargetView* rtv, [[maybe_unused]] ClearFlag flags, [[maybe_unused]] float depthValue, [[maybe_unused]] uint8_t stencilValue)
    {
    }

    void RenderCmdListImpl::SetComputeRootSignature([[maybe_unused]] const RootSignature* signature)
    {
    }

    void RenderCmdListImpl::SetComputePipeline([[maybe_unused]] const ComputePipeline* pipeline)
    {
    }

    void RenderCmdListImpl::SetComputeDescriptorTable([[maybe_unused]] uint32_t slot, [[maybe_unused]] const DescriptorTable* table)
    {
    }

    void RenderCmdListImpl::SetComputeConstantBuffer([[maybe_unused]] uint32_t slot, [[maybe_unused]] const Buffer* buffer, [[maybe_unused]] uint32_t offset)
    {
    }

    void RenderCmdListImpl::SetCompute32BitConstantValue([[maybe_unused]] uint32_t slot, [[maybe_unused]] uint32_t value, [[maybe_unused]] uint32_t offset)
    {
    }

    void RenderCmdListImpl::SetCompute32BitConstantValues([[maybe_unused]] uint32_t slot, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t count, [[maybe_unused]] uint32_t offset)
    {
    }

    void RenderCmdListImpl::Dispatch([[maybe_unused]] uint32_t countX, [[maybe_unused]] uint32_t countY, [[maybe_unused]] uint32_t countZ)
    {
    }

    void RenderCmdListImpl::TransitionBarrier([[maybe_unused]] const Texture* texture, [[maybe_unused]] TextureState oldState, [[maybe_unused]] TextureState newState, [[maybe_unused]] uint32_t subresource)
    {
    }

    void RenderCmdListImpl::TransitionBarrier([[maybe_unused]] const Buffer* buffer, [[maybe_unused]] BufferState oldState, [[maybe_unused]] BufferState newState)
    {
    }

    void RenderCmdListImpl::RWBarrier([[maybe_unused]] const Buffer* buffer)
    {
    }

    void RenderCmdListImpl::RWBarrier([[maybe_unused]] const Texture* texture)
    {
    }

    void RenderCmdListImpl::BeginRenderPass([[maybe_unused]] const RenderPassDesc& desc)
    {
    }

    void RenderCmdListImpl::EndRenderPass()
    {
    }

    void RenderCmdListImpl::Resolve([[maybe_unused]] const RenderTargetView* srcView, [[maybe_unused]] const RenderTargetView* dstView)
    {
    }

    void RenderCmdListImpl::SetRenderRootSignature([[maybe_unused]] const RootSignature* rootSignature)
    {
    }

    void RenderCmdListImpl::SetRenderPipeline([[maybe_unused]] const RenderPipeline* pipeline)
    {
    }

    void RenderCmdListImpl::SetRenderDescriptorTable([[maybe_unused]] uint32_t slot, [[maybe_unused]] const DescriptorTable* table)
    {
    }

    void RenderCmdListImpl::SetRenderConstantBuffer([[maybe_unused]] uint32_t slot, [[maybe_unused]] const Buffer* buffer, [[maybe_unused]] uint32_t offset)
    {
    }

    void RenderCmdListImpl::SetRender32BitConstantValue([[maybe_unused]] uint32_t slot, [[maybe_unused]] uint32_t value, [[maybe_unused]] uint32_t offset)
    {
    }

    void RenderCmdListImpl::SetRender32BitConstantValues([[maybe_unused]] uint32_t slot, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t count, [[maybe_unused]] uint32_t offset)
    {
    }

    void RenderCmdListImpl::SetIndexBuffer([[maybe_unused]] const Buffer* buffer, [[maybe_unused]] uint32_t offset, [[maybe_unused]] uint32_t size, [[maybe_unused]] IndexType type)
    {
    }

    void RenderCmdListImpl::SetVertexBuffer([[maybe_unused]] uint32_t index, [[maybe_unused]] const VertexBufferFormat* vbf, [[maybe_unused]] const Buffer* buffer, [[maybe_unused]] uint32_t offset, [[maybe_unused]] uint32_t size)
    {
    }

    void RenderCmdListImpl::SetVertexBuffers([[maybe_unused]] uint32_t index, [[maybe_unused]] uint32_t count, [[maybe_unused]] const VertexBufferFormat* const* vbfs, [[maybe_unused]] const Buffer* const* buffers, [[maybe_unused]] const uint32_t* offsets, [[maybe_unused]] const uint32_t* sizes)
    {
    }

    void RenderCmdListImpl::SetViewport([[maybe_unused]] const Viewport& viewport)
    {
    }

    void RenderCmdListImpl::SetScissor([[maybe_unused]] const Vec2u& pos, [[maybe_unused]] const Vec2u& size)
    {
    }

    void RenderCmdListImpl::SetStencilRef([[maybe_unused]] uint8_t value)
    {
    }

    void RenderCmdListImpl::SetBlendColor([[maybe_unused]] const Vec4f& color)
    {
    }

    void RenderCmdListImpl::SetDepthBounds([[maybe_unused]] float min, [[maybe_unused]] float max)
    {
    }

    void RenderCmdListImpl::Draw([[maybe_unused]] const DrawDesc& desc)
    {
    }

    void RenderCmdListImpl::DrawIndexed([[maybe_unused]] const DrawIndexedDesc& desc)
    {
    }
}

#endif
