// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/rhi/types.h"

namespace he::rhi
{
    /// Base command list interface shared by all command list types.
    class CmdList
    {
    public:
        virtual ~CmdList() = default;

        /// Begin recording of commands into this list.
        ///
        /// \param[in] alloc The command allocator to use for allocating commands in this list.
        /// \return The result of the operation.
        virtual Result Begin(CmdAllocator* alloc) = 0;

        /// End recording of commands into this list.
        ///
        /// \return The result of the operation.
        virtual Result End() = 0;

        /// Mark the beginning of a group of commands. This is primarily useful for debugging
        /// since these will group commands in tools like PIX or RenderDoc.
        ///
        /// \param [in] msg The string name of the group.
        /// \return The result of the operation.
        virtual void BeginGroup(const char* msg) = 0;

        /// Mark the end of the last opened group of commands.
        ///
        /// \return The result of the operation.
        virtual void EndGroup() = 0;

        /// Sets a debug marker that show up in tools like PIX or RenderDoc.
        ///
        /// \return The result of the operation.
        virtual void SetMarker(const char* msg) = 0;

        /// Writes a GPU timestamp into the query set at the specified index.
        ///
        /// \param[in] querySet The query set to write into.
        /// \param[in] index The index in the query set to write.
        virtual void WriteTimestamp(const TimestampQuerySet* querySet, uint32_t index) = 0;

        /// Resolves a range of timestamp queries into a buffer.
        ///
        /// \param[in] querySet The query set to resolve.
        /// \param[in] firstIndex The first query index to resolve.
        /// \param[in] count The number of query indices to resolve.
        /// \param[in] dst The destination buffer to write the timestamps into.
        /// \param[in] dstOffset The destination byte offset in the buffer.
        virtual void ResolveTimestamps(const TimestampQuerySet* querySet, uint32_t firstIndex, uint32_t count, const Buffer* dst, uint32_t dstOffset = 0) = 0;
    };

    /// Command list that can perform copy operations.
    class CopyCmdList : public CmdList
    {
    public:
        /// Copies the contents of the source buffer to the destination buffer.
        ///
        /// \param[in] src The source to copy from.
        /// \param[in] dst The destination to copy to.
        /// \param[in] region Optional. Controls the region of the buffers to copy. By default
        ///     the entire buffer is copied.
        virtual void Copy(const Buffer* src, const Buffer* dst, const BufferCopy* region = nullptr) = 0;

        /// Copies the contents of the source texture to the destination texture.
        ///
        /// \param[in] src The source to copy from.
        /// \param[in] dst The destination to copy to.
        /// \param[in] region Optional. Controls the region of the textures to copy. By default
        ///     the entire texture is copied.
        virtual void Copy(const Texture* src, const Texture* dst, const TextureCopy* region = nullptr) = 0;

        /// Copies the contents of the source buffer to the destination texture.
        ///
        /// \param[in] src The source to copy from.
        /// \param[in] dst The destination to copy to.
        /// \param[in] region Controls how the copy is performed.
        virtual void Copy(const Buffer* src, const Texture* dst, const BufferTextureCopy& region) = 0;

        /// Copies the contents of the source texture to the destination buffer.
        ///
        /// \param[in] src The source to copy from.
        /// \param[in] dst The destination to copy to.
        /// \param[in] region Controls how the copy is performed.
        virtual void Copy(const Texture* src, const Buffer* dst, const BufferTextureCopy& region) = 0;
    };

    /// Command list that can perform compute and copy operations.
    class ComputeCmdList : public CopyCmdList
    {
    public:
        /// Clears the contents of the buffer.
        ///
        /// \param[in] view The view to clear.
        /// \param[in] values The four float components to set the values of the view to.
        virtual void Clear(const RWBufferView* view, const Vec4f& values) = 0;

        /// Clears the contents of the buffer.
        ///
        /// \param[in] view The view to clear.
        /// \param[in] values The four unsigned integer components to set the values of the view to.
        virtual void Clear(const RWBufferView* view, const Vec4u& values) = 0;

        /// Clears the contents of the texture.
        ///
        /// \param[in] view The view to clear.
        /// \param[in] values The four float components to set the values of the view to.
        virtual void Clear(const RWTextureView* view, const Vec4f& values) = 0;

        /// Clears the contents of the texture.
        ///
        /// \param[in] view The view to clear.
        /// \param[in] values The four unsigned integer components to set the values of the view to.
        virtual void Clear(const RWTextureView* view, const Vec4u& values) = 0;

        /// Sets the compute root signature.
        ///
        /// \param[in] signature The root signature to set the compute layout to.
        virtual void SetComputeRootSignature(const RootSignature* signature) = 0;

        /// Sets the compute pipeline state.
        ///
        /// \param[in] pipeline The pipeline state to enable.
        virtual void SetComputePipeline(const ComputePipeline* pipeline) = 0;

        /// Sets a descriptor table into the compute root signature.
        ///
        /// \param[in] slot The root signature slot to set the table into.
        /// \param[in] table The table to set into the slot.
        virtual void SetComputeDescriptorTable(uint32_t slot, const DescriptorTable* table) = 0;

        /// Sets a constant buffer into the compute root signature.
        ///
        /// \param[in] slot The root signature slot to set the buffer into.
        /// \param[in] buffer The buffer to set into the slot.
        /// \param[in] offset The offset (in bytes) within the buffer to set into the slot.
        virtual void SetComputeConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset = 0) = 0;

        /// Sets a constant 32-bit value into the compute root signature.
        ///
        /// \param[in] slot The root signature slot to set the value into.
        /// \param[in] value The value to set into the slot.
        /// \param[in] offset Optional. The offset (in number of values) to set.
        virtual void SetCompute32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset = 0) = 0;

        /// Sets a series of constant 32-bit values into the compute root signature.
        ///
        /// \param[in] slot The root signature slot to set the value into.
        /// \param[in] data A pointer to the data of the constant values.
        /// \param[in] count The number of values to set into the slot.
        /// \param[in] offset Optional. The offset (in number of values) to set.
        virtual void SetCompute32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset = 0) = 0;

        /// Executes a command list from a thread group.
        ///
        /// \param[in] countX The number of groups dispatched in the x direction.
        /// \param[in] countY The number of groups dispatched in the y direction.
        /// \param[in] countZ The number of groups dispatched in the z direction.
        virtual void Dispatch(uint32_t countX, uint32_t countY, uint32_t countZ) = 0;

        /// Add a transition barrier that indicates a set of subresources are being transitioned
        /// to different a usage.
        ///
        /// \param[in] texture The texture to transition.
        /// \param[in] oldState The old state to transition from.
        /// \param[in] newState The new state to transition to.
        /// \param[in] subresource Optional. The subresources of the texture to transition.
        ///     By default all subresources are transitioned.
        virtual void TransitionBarrier(const Texture* texture, TextureState oldState, TextureState newState, uint32_t subresource = AllSubresources) = 0;

        /// Add a transition barrier that indicates a buffer is being transitioned
        /// to different a usage.
        ///
        /// \param[in] buffer The buffer to transition.
        /// \param[in] oldState The old state to transition from.
        /// \param[in] newState The new state to transition to.
        virtual void TransitionBarrier(const Buffer* buffer, BufferState oldState, BufferState newState) = 0;

        /// Add a barrier that indicates all reads and writes to a buffer must complete before
        /// any future reads or writes can begin.
        ///
        /// \param[in] buffer The buffer to add the barrier for.
        virtual void RWBarrier(const Buffer* buffer) = 0;

        /// Add a barrier that indicates all reads and writes to a texture must complete before
        /// any future reads or writes can begin.
        ///
        /// \param[in] texture The texture to add the barrier for.
        virtual void RWBarrier(const Texture* texture) = 0;
    };

    /// Command list that can perform render, compute, and copy operations.
    class RenderCmdList : public ComputeCmdList
    {
    public:
        using ComputeCmdList::Clear;

        /// Clears the contents of the render target view.
        ///
        /// \param[in] rtv The render target view to clear.
        /// \param[in] values The four float components to set the values of the view to.
        virtual void Clear(const RenderTargetView* rtv, const Vec4f& values) = 0;

        /// Clears the contents of a depth-stencil render target view.
        ///
        /// \param[in] rtv The render target view to clear.
        /// \param[in] flags Which planes (depth and/or stencil) to clear. Must specify one or both.
        /// \param[in] depthValue The value to clear the depth buffer to.
        /// \param[in] stencilValue The value to clear the stencil buffer to.
        virtual void Clear(const RenderTargetView* rtv, ClearFlag flags, float depthValue, uint8_t stencilValue) = 0;

        /// Begins a render pass performing any discards, clears, or transitions as necessary;
        /// as well as setting the render targets for the pass.
        ///
        /// \param[in] desc Controls what actions are taken when starting the new render pass.
        virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;

        /// Ends a render pass performing any discards or resolves as necessary.
        virtual void EndRenderPass() = 0;

        /// Copy a multi-sampled resource into a non-multi-sampled resource.
        ///
        /// \param[in] src The source view to copy from.
        /// \param[in] dst The destination view to copy to.
        virtual void Resolve(const RenderTargetView* src, const RenderTargetView* dst) = 0;

        /// Sets the render root signature.
        ///
        /// \param[in] signature The root signature to set the render layout to.
        virtual void SetRenderRootSignature(const RootSignature* rootSignature) = 0;

        /// Sets the render pipeline state.
        ///
        /// \param[in] pipeline The pipeline state to enable.
        virtual void SetRenderPipeline(const RenderPipeline* pipeline) = 0;

        /// Sets a descriptor table into the render root signature.
        ///
        /// \param[in] slot The root signature slot to set the table into.
        /// \param[in] table The table to set into the slot.
        virtual void SetRenderDescriptorTable(uint32_t slot, const DescriptorTable* table) = 0;

        /// Sets a constant buffer into the render root signature.
        ///
        /// \param[in] slot The root signature slot to set the buffer into.
        /// \param[in] buffer The buffer to set into the slot.
        /// \param[in] offset The offset (in bytes) within the buffer to set into the slot.
        virtual void SetRenderConstantBuffer(uint32_t slot, const Buffer* buffer, uint32_t offset = 0) = 0;

        /// Sets a constant 32-bit value into the render root signature.
        ///
        /// \param[in] slot The root signature slot to set the value into.
        /// \param[in] value The value to set into the slot.
        /// \param[in] offset Optional. The offset (in number of values) to set.
        virtual void SetRender32BitConstantValue(uint32_t slot, uint32_t value, uint32_t offset = 0) = 0;

        /// Sets a series of constant 32-bit values into the render root signature.
        ///
        /// \param[in] slot The root signature slot to set the value into.
        /// \param[in] data A pointer to the data of the constant values.
        /// \param[in] count The number of values to set into the slot.
        /// \param[in] offset Optional. The offset (in number of values) to set.
        virtual void SetRender32BitConstantValues(uint32_t slot, const void* data, uint32_t count, uint32_t offset = 0) = 0;

        /// Sets a buffer as the index buffer for the input-assembler stage.
        ///
        /// \param[in] buffer The buffer to bind.
        /// \param[in] offset The offset (in bytes) in the buffer to the first index to use.
        /// \param[in] size The size (in bytes) of the buffer.
        /// \param[in] type The index buffer format to interpret each index as.
        virtual void SetIndexBuffer(const Buffer* buffer, uint32_t offset, uint32_t size, IndexType type) = 0;

        /// Sets a buffer as a vertex buffer in the pipeline.
        ///
        /// \param[in] index The index in the device to set the vertex buffer to.
        /// \param[in] vbf The format of the vertex buffer.
        /// \param[in] buffer The buffer to set as the vertex buffer.
        /// \param[in] offset The offset (in bytes) in the buffer to the first vertex to use.
        /// \param[in] size The size (in bytes) of the buffer.
        virtual void SetVertexBuffer(uint32_t index, const VertexBufferFormat* vbf, const Buffer* buffer, uint32_t offset, uint32_t size) = 0;

        /// Sets a series of buffers as vertex buffers in the pipeline.
        ///
        /// \param[in] index The starting index in the device to set the vertex buffers to.
        /// \param[in] count The number of buffers to set.
        /// \param[in] vbfs Array of formats of the vertex buffers.
        /// \param[in] buffers Array of buffers to set as the vertex buffers.
        /// \param[in] offsets Array of offsets (in bytes) in each buffer to the first vertex to use.
        /// \param[in] sizes Array of sizes (in bytes) of each buffer.
        virtual void SetVertexBuffers(uint32_t index, uint32_t count, const VertexBufferFormat* const* vbfs, const Buffer* const* buffers, const uint32_t* offsets, const uint32_t* sizes) = 0;

        /// Sets the viewport for the raster stage.
        ///
        /// \param[in] viewport The viewport to set.
        virtual void SetViewport(const Viewport& viewport) = 0;

        /// Sets the scissor rectangle for the raster stage.
        ///
        /// \param[in] pos The position of the top-left corner of the scissor rectangle.
        /// \param[in] size The size of the scissor rectangle.
        virtual void SetScissor(const Vec2u& pos, const Vec2u& size) = 0;

        /// Sets the reference value for depth stencil tests.
        ///
        /// \param[in] value Reference value to perform against when doing a depth-stencil test.
        virtual void SetStencilRef(uint8_t value) = 0;

        /// Sets the blend factor that modulate values for a pixel shader, render target, or both.
        ///
        /// \param[in] color The RGBA components to set as the blend color.
        virtual void SetBlendColor(const Vec4f& color) = 0;

        /// Sets the bounds of the depth-bounds tests that will discard pixels outside the range
        /// specified by [min, max].
        ///
        /// \param[in] min The minimum value for a pixel not to be discarded.
        /// \param[in] max The maximum value before a pixel is discarded.
        virtual void SetDepthBounds(float min, float max) = 0;

        /// Draw non-indexed, instanced primitives.
        ///
        /// \param[in] desc Controls the behavior of the draw.
        virtual void Draw(const DrawDesc& desc) = 0;

        /// Draw indexed, instanced primitives.
        ///
        /// \param[in] desc Controls the behavior of the draw.
        virtual void DrawIndexed(const DrawIndexedDesc& desc) = 0;
    };
}
