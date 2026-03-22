// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/result.h"
#include "he/rhi/types.h"

namespace he::rhi
{
    /// Interface for a rendering device that provides access to a virtual adapter on the system.
    /// Used to create device resources such as buffers, textures, and command lists.
    class Device
    {
    public:
        virtual ~Device() = default;

        /// Creates a buffer resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateBuffer(const BufferDesc& desc, Buffer*& out) = 0;

        /// Destroys a buffer resource created with \ref CreateBuffer.
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] buffer The resource to destroy.
        virtual void DestroyBuffer(Buffer* buffer) = 0;

        /// Creates a read-only buffer view resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateBufferView(const BufferViewDesc& desc, BufferView*& out) = 0;

        /// Destroys a buffer view resource created with \ref CreateBufferView
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] view The resource to destroy.
        virtual void DestroyBufferView(BufferView* view) = 0;

        /// Creates a read-write buffer view resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateRWBufferView(const BufferViewDesc& desc, RWBufferView*& out) = 0;

        /// Destroys a read-write buffer view resource created with \ref CreateRWBufferView
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] view The resource to destroy.
        virtual void DestroyRWBufferView(RWBufferView* view) = 0;

        /// Gets a CPU pointer to the buffer resource.
        ///
        /// \param[in] buffer The buffer resource to get a pointer to.
        /// \param[in] offset Optional. The offset (in bytes) within the buffer to map.
        /// \param[in] size Optional. The size (in bytes) of the section of the buffer to map.
        ///     Passing zero (default) will map from offset until to the end of the buffer.
        /// \return A pointer to the mapped buffer.
        virtual void* Map(Buffer* buffer, uint32_t offset = 0, uint32_t size = 0) = 0;

        /// Invalidates the mapped CPU pointer to the buffer.
        ///
        /// \param[in] buffer The buffer to unmap from the CPU.
        virtual void Unmap(Buffer* buffer) = 0;

        /// Creates a copy command list.
        ///
        /// \param[in] desc The descriptor for how to create the command list.
        /// \param[out] out A pointer to the newly created command list.
        /// \return The result of the operation.
        virtual Result CreateCopyCmdList(const CmdListDesc& desc, CopyCmdList*& out) = 0;

        /// Destroys a copy command list created with \ref CreateCopyCmdList
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] cmdList The copy command list to destroy.
        virtual void DestroyCopyCmdList(CopyCmdList* cmdList) = 0;

        /// Creates a compute command list.
        ///
        /// \param[in] desc The descriptor for how to create the command list.
        /// \param[out] out A pointer to the newly created command list.
        /// \return The result of the operation.
        virtual Result CreateComputeCmdList(const CmdListDesc& desc, ComputeCmdList*& out) = 0;

        /// Destroys a compute command list created with \ref CreateComputeCmdList
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] cmdList The compute command list to destroy.
        virtual void DestroyComputeCmdList(ComputeCmdList* cmdList) = 0;

        /// Creates a render command list.
        ///
        /// \param[in] desc The descriptor for how to create the command list.
        /// \param[out] out A pointer to the newly created command list.
        /// \return The result of the operation.
        virtual Result CreateRenderCmdList(const CmdListDesc& desc, RenderCmdList*& out) = 0;

        /// Destroys a render command list created with \ref CreateRenderCmdList
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] cmdList The render command list to destroy.
        virtual void DestroyRenderCmdList(RenderCmdList* cmdList) = 0;

        /// Creates a command allocator.
        ///
        /// \param[in] desc The descriptor for how to create the allocator.
        /// \param[out] out A pointer to the newly created allocator.
        /// \return The result of the operation.
        virtual Result CreateCmdAllocator(const CmdAllocatorDesc& desc, CmdAllocator*& out) = 0;

        /// Destroys a command allocator created with \ref CreateCmdAllocator
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] alloc The allocator to destroy.
        virtual void DestroyCmdAllocator(CmdAllocator* alloc) = 0;

        /// Resets the command allocator, releasing memory to that it can be reused.
        /// A command allocator can only be reset after all commands allocated by it have
        /// completed execution on the GPU.
        ///
        /// \param[in] alloc The allocator to reset.
        /// \return The result of the operation.
        virtual Result ResetCmdAllocator(CmdAllocator* alloc) = 0;

        /// Gets the copy command queue that can be used to submit copy command lists.
        ///
        /// \return The device's copy queue.
        virtual CopyCmdQueue& GetCopyCmdQueue() = 0;

        /// Gets the compute command queue that can be used to submit compute command lists.
        ///
        /// \return The device's compute queue.
        virtual ComputeCmdQueue& GetComputeCmdQueue() = 0;

        /// Gets the render command queue that can be used to submit render command lists.
        ///
        /// \return The device's render queue.
        virtual RenderCmdQueue& GetRenderCmdQueue() = 0;

        /// Creates a descriptor table that can be placed in a root signature slot.
        ///
        /// \param[in] desc The descriptor for how to create the descriptor table.
        /// \param[out] out A pointer to the newly created descriptor table.
        /// \return The result of the operation.
        virtual Result CreateDescriptorTable(const DescriptorTableDesc& desc, DescriptorTable*& out) = 0;

        /// Destroys a descriptor table created with \ref CreateDescriptorTable
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] table The descriptor table to destroy.
        virtual void DestroyDescriptorTable(DescriptorTable* table) = 0;

        /// Sets buffer views into descriptor ranges in the table.
        ///
        /// \note Entries must be set before the table is used in a command list.
        ///
        /// \param[in] table The descriptor table to set the views into.
        /// \param[in] rangeIndex Index of the descriptor range to set the views into.
        /// \param[in] descIndex Starting index of the descriptor to set.
        /// \param[in] count The number of views to set.
        /// \param[in] views Array of views to set. Can contain nullptr to unbind a resource
        ///     but behavior is undefined if it is accessed from the shader.
        virtual void SetBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const BufferView* const* views) = 0;

        /// Sets texture views into descriptor ranges in the table.
        ///
        /// \note Entries must be set before the table is used in a command list.
        ///
        /// \param[in] table The descriptor table to set the views into.
        /// \param[in] rangeIndex Index of the descriptor range to set the views into.
        /// \param[in] descIndex Starting index of the descriptor to set.
        /// \param[in] count The number of views to set.
        /// \param[in] views Array of views to set. Can contain nullptr to unbind a resource
        ///     but behavior is undefined if it is accessed from the shader.
        virtual void SetTextureViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const TextureView* const* views) = 0;

        /// Sets read-write buffer views into descriptor ranges in the table.
        ///
        /// \note Entries must be set before the table is used in a command list.
        ///
        /// \param[in] table The descriptor table to set the views into.
        /// \param[in] rangeIndex Index of the descriptor range to set the views into.
        /// \param[in] descIndex Starting index of the descriptor to set.
        /// \param[in] count The number of views to set.
        /// \param[in] views Array of views to set. Can contain nullptr to unbind a resource
        ///     but behavior is undefined if it is accessed from the shader.
        virtual void SetRWBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWBufferView* const* views) = 0;

        /// Sets read-write texture views into descriptor ranges in the table.
        ///
        /// \note Entries must be set before the table is used in a command list.
        ///
        /// \param[in] table The descriptor table to set the views into.
        /// \param[in] rangeIndex Index of the descriptor range to set the views into.
        /// \param[in] descIndex Starting index of the descriptor to set.
        /// \param[in] count The number of views to set.
        /// \param[in] views Array of views to set. Can contain nullptr to unbind a resource
        ///     but behavior is undefined if it is accessed from the shader.
        virtual void SetRWTextureViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const RWTextureView* const* views) = 0;

        /// Sets constant buffer views into descriptor ranges in the table.
        ///
        /// \note Entries must be set before the table is used in a command list.
        ///
        /// \param[in] table The descriptor table to set the views into.
        /// \param[in] rangeIndex Index of the descriptor range to set the views into.
        /// \param[in] descIndex Starting index of the descriptor to set.
        /// \param[in] count The number of views to set.
        /// \param[in] views Array of views to set. Can contain nullptr to unbind a resource
        ///     but behavior is undefined if it is accessed from the shader.
        virtual void SetConstantBufferViews(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const ConstantBufferView* const* views) = 0;

        /// Sets samplers into descriptor ranges in the table.
        ///
        /// \note Entries must be set before the table is used in a command list.
        ///
        /// \param[in] table The descriptor table to set the samplers into.
        /// \param[in] rangeIndex Index of the descriptor range to set the samplers into.
        /// \param[in] descIndex Starting index of the descriptor to set.
        /// \param[in] count The number of samplers to set.
        /// \param[in] samplers Array of samplers to set. Can contain nullptr to unbind a resource
        ///     but behavior is undefined if it is accessed from the shader.
        virtual void SetSamplers(DescriptorTable* table, uint32_t rangeIndex, uint32_t descIndex, uint32_t count, const Sampler* const* samplers) = 0;

        /// Creates a CPU fence.
        ///
        /// \param[in] desc The descriptor for how to create the fence.
        /// \param[out] out A pointer to the newly created fence.
        /// \return The result of the operation.
        virtual Result CreateCpuFence(const CpuFenceDesc& desc, CpuFence*& out) = 0;

        /// Destroys a CPU fence created with \ref CreateCpuFence
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] fence The fence to destroy.
        virtual void DestroyCpuFence(CpuFence* fence) = 0;

        /// Creates a GPU fence.
        ///
        /// \param[in] desc The descriptor for how to create the fence.
        /// \param[out] out A pointer to the newly created fence.
        /// \return The result of the operation.
        virtual Result CreateGpuFence(const GpuFenceDesc& desc, GpuFence*& out) = 0;

        /// Destroys a GPU fence created with \ref CreateGpuFence
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] fence The fence to destroy.
        virtual void DestroyGpuFence(GpuFence* fence) = 0;

        /// Waits for a CPU fence to be singaled by the GPU.
        ///
        /// \param[in] fence The fence to wait on.
        /// \param[in] timeoutMs The descriptor for how to create the fence.
        /// \param[out] out A pointer to the newly created fence.
        /// \return The result of the operation.
        virtual bool WaitForFence(const CpuFence* fence, uint32_t timeoutMs = static_cast<uint32_t>(-1)) = 0;

        /// Checks if a CPU fence has been signaled and returns immediately.
        ///
        /// \param[in] fence The fence to check.
        /// \return True if the fence has been signaled, false otherwise.
        virtual bool IsFenceSignaled(const CpuFence* fence) = 0;

        /// Gets the current value of the fence.
        ///
        /// \param[in] fence The fence to get the current value of.
        /// \return The current value of the fence, or UINT64_MAX if the device has been removed.
        virtual uint64_t GetFenceValue(const GpuFence* fence) = 0;

        /// Creates a timestamp query set.
        ///
        /// \param[in] desc The descriptor for how to create the query set.
        /// \param[out] out A pointer to the newly created query set.
        /// \return The result of the operation.
        virtual Result CreateTimestampQuerySet(const TimestampQuerySetDesc& desc, TimestampQuerySet*& out) = 0;

        /// Destroys a timestamp query set created with \ref CreateTimestampQuerySet.
        ///
        /// \param[in] querySet The query set to destroy.
        virtual void DestroyTimestampQuerySet(TimestampQuerySet* querySet) = 0;

        /// Creates a compute pipeline.
        ///
        /// \param[in] desc The descriptor for how to create the pipeline.
        /// \param[out] out A pointer to the newly created pipeline.
        /// \return The result of the operation.
        virtual Result CreateComputePipeline(const ComputePipelineDesc& desc, ComputePipeline*& out) = 0;

        /// Destroys a compute pipeline created with \ref CreateComputePipeline
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] pipeline The pipeline to destroy.
        virtual void DestroyComputePipeline(ComputePipeline* pipeline) = 0;

        /// Creates a render pipeline.
        ///
        /// \param[in] desc The descriptor for how to create the pipeline.
        /// \param[out] out A pointer to the newly created pipeline.
        /// \return The result of the operation.
        virtual Result CreateRenderPipeline(const RenderPipelineDesc& desc, RenderPipeline*& out) = 0;

        /// Destroys a render pipeline created with \ref CreateRenderPipeline
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] pipeline The pipeline to destroy.
        virtual void DestroyRenderPipeline(RenderPipeline* pipeline) = 0;

        /// Creates a root signature.
        ///
        /// \param[in] desc The descriptor for how to create the signature.
        /// \param[out] out A pointer to the newly created signature.
        /// \return The result of the operation.
        virtual Result CreateRootSignature(const RootSignatureDesc& desc, RootSignature*& out) = 0;

        /// Destroys a root signature created with \ref CreateRootSignature
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] signature The signature to destroy.
        virtual void DestroyRootSignature(RootSignature* signature) = 0;

        /// Creates a texture sampler.
        ///
        /// \param[in] desc The descriptor for how to create the sampler.
        /// \param[out] out A pointer to the newly created sampler.
        /// \return The result of the operation.
        virtual Result CreateSampler(const SamplerDesc& desc, Sampler*& out) = 0;

        /// Destroys a sampler created with \ref CreateSampler
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] sampler The sampler to destroy.
        virtual void DestroySampler(Sampler* sampler) = 0;

        /// Creates a shader.
        ///
        /// \param[in] desc The descriptor for how to create the shader.
        /// \param[out] out A pointer to the newly created shader.
        /// \return The result of the operation.
        virtual Result CreateShader(const ShaderDesc& desc, Shader*& out) = 0;

        /// Destroys a shader created with \ref CreateShader
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] shader The shader to destroy.
        virtual void DestroyShader(Shader* shader) = 0;

        /// Creates a swap chain.
        ///
        /// \param[in] desc The descriptor for how to create the swap chain.
        /// \param[out] out A pointer to the newly created swap chain.
        /// \return The result of the operation.
        virtual Result CreateSwapChain(const SwapChainDesc& desc, SwapChain*& out) = 0;

        /// Destroys a swap chain created with \ref CreateSwapChain
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] swapChain The swap chain to destroy.
        virtual void DestroySwapChain(SwapChain* swapChain) = 0;

        /// Updates a swap chain to match the new descriptor.
        ///
        /// \note This can cause a stall as the render command queue is flushed.
        ///
        /// \param[in] swapChain The swap chain to be updated.
        /// \param[in] desc The new descriptor to update the swap chain to.
        /// \return The result of the update operation.
        virtual Result UpdateSwapChain(SwapChain* swapChain, const SwapChainDesc& desc) = 0;

        /// Waits for the next back buffer to be available and returns it.
        ///
        /// \param[in] swapChain The swap chain to get the next backbuffer for.
        /// \return The texture and render target view that is the next present target.
        virtual PresentTarget AcquirePresentTarget(SwapChain* swapChain) = 0;

        /// Checks if the swap chain is currently in fullscreen exclusive mode.
        ///
        /// \param[in] swapChain The swap chain to check.
        /// \return True if the swap chain is in FSE mode, false otherwise.
        virtual bool IsFullscreen(SwapChain* swapChain) = 0;

        /// Transitions the swap chain in or out of fullscreen exclusive mode.
        ///
        /// \param[in] swapChain The swap chain to check.
        /// \param[in] fullscreen True to set the swap chain to FSE mode, false to make it windowed.
        /// \return The result of the operation. May fail if the operation is already in progress.
        virtual Result SetFullscreen(SwapChain* swapChain, bool fullscreen) = 0;

        /// Creates a texture resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateTexture(const TextureDesc& desc, Texture*& out) = 0;

        /// Destroys a texture resource created with \ref CreateTexture.
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] texture The resource to destroy.
        virtual void DestroyTexture(Texture* texture) = 0;

        /// Creates a read-only texture view resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateTextureView(const TextureViewDesc& desc, TextureView*& out) = 0;

        /// Destroys a read-only texture view resource created with \ref CreateTextureView.
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] view The resource to destroy.
        virtual void DestroyTextureView(TextureView* view) = 0;

        /// Creates a read-write texture view resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateRWTextureView(const TextureViewDesc& desc, RWTextureView*& out) = 0;

        /// Destroys a read-write texture view resource created with \ref CreateRWTextureView.
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] view The resource to destroy.
        virtual void DestroyRWTextureView(RWTextureView* view) = 0;

        /// Creates a render target view resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateRenderTargetView(const TextureViewDesc& desc, RenderTargetView*& out) = 0;

        /// Destroys a render target view resource created with \ref CreateRenderTargetView.
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] view The resource to destroy.
        virtual void DestroyRenderTargetView(RenderTargetView* view) = 0;

        /// Creates a constant buffer view resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateConstantBufferView(const ConstantBufferViewDesc& desc, ConstantBufferView*& out) = 0;

        /// Destroys a constant buffer view resource created with \ref CreateConstantBufferView.
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] view The resource to destroy.
        virtual void DestroyConstantBufferView(ConstantBufferView* view) = 0;

        /// Creates a vertex buffer format resource.
        ///
        /// \param[in] desc The descriptor for how to create the resource.
        /// \param[out] out A pointer to the newly created resource.
        /// \return The result of the operation.
        virtual Result CreateVertexBufferFormat(const VertexBufferFormatDesc& desc, VertexBufferFormat*& out) = 0;

        /// Destroys a vertex buffer format resource created with \ref CreateVertexBufferFormat.
        ///
        /// \note You can use \ref SafeDestroy instead which will check your pointer for validity
        /// before trying to destroy it, and then set it to nullptr after destroying it.
        ///
        /// \param[in] vbf The resource to destroy.
        virtual void DestroyVertexBufferFormat(VertexBufferFormat* vbf) = 0;

        /// Gets the capabilities of the device.
        ///
        /// \return The device capabilities.
        virtual const DeviceInfo& GetDeviceInfo() = 0;

        /// Gets the valid swap chain formats the output device supports.
        ///
        /// \note The expected way to use this function is to call it twice. First with nullptr
        /// as the `formats` parameter to get the count, then allocate the necessary space, then
        /// call it again with the properly sized array.
        ///
        /// \param[in] nvh The native view handle for the platform view.
        /// \param[out] formats Array of formats to write to. Pass null to just get the count.
        /// \param[in,out] count The length of the `formats` array to write to. This function
        ///     will write the number of detected formats upon return, even if it is larger than
        ///     the length of the `formats` array.
        virtual Result GetSwapChainFormats(void* nvh, SwapChainFormat* formats, uint32_t& count) = 0;

        /// Safely destroys a device resource by checking for nullptr before attempting to destroy
        /// the object, then setting the passed in variable to nullptr after destroying.
        ///
        /// \param[in] x The resource to destroy.
        template <typename T>
        void SafeDestroy(T*& x);
    };

    template <> inline void Device::SafeDestroy(Buffer*& x) { if (x) { DestroyBuffer(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(BufferView*& x) { if (x) { DestroyBufferView(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(RWBufferView*& x) { if (x) { DestroyRWBufferView(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(CopyCmdList*& x) { if (x) { DestroyCopyCmdList(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(ComputeCmdList*& x) { if (x) { DestroyComputeCmdList(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(ConstantBufferView*& x) { if (x) { DestroyConstantBufferView(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(RenderCmdList*& x) { if (x) { DestroyRenderCmdList(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(CmdAllocator*& x) { if (x) { DestroyCmdAllocator(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(DescriptorTable*& x) { if (x) { DestroyDescriptorTable(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(CpuFence*& x) { if (x) { DestroyCpuFence(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(GpuFence*& x) { if (x) { DestroyGpuFence(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(TimestampQuerySet*& x) { if (x) { DestroyTimestampQuerySet(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(ComputePipeline*& x) { if (x) { DestroyComputePipeline(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(RenderPipeline*& x) { if (x) { DestroyRenderPipeline(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(RootSignature*& x) { if (x) { DestroyRootSignature(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(Sampler*& x) { if (x) { DestroySampler(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(Shader*& x) { if (x) { DestroyShader(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(SwapChain*& x) { if (x) { DestroySwapChain(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(Texture*& x) { if (x) { DestroyTexture(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(TextureView*& x) { if (x) { DestroyTextureView(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(RWTextureView*& x) { if (x) { DestroyRWTextureView(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(RenderTargetView*& x) { if (x) { DestroyRenderTargetView(x); } x = nullptr; }
    template <> inline void Device::SafeDestroy(VertexBufferFormat*& x) { if (x) { DestroyVertexBufferFormat(x); } x = nullptr; }
}
