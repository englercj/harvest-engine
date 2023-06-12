// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/enum_ops.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/math/types.h"
#include "he/rhi/config.h"

#if HE_RHI_ENABLE_NAMES
    #define HE_RHI_NAME_MEMBER()    const char* name{ nullptr }
    #define HE_RHI_SET_NAME(x, n)   x.name = n
#else
    #define HE_RHI_NAME_MEMBER()
    #define HE_RHI_SET_NAME(...)
#endif

namespace he::rhi
{
    // --------------------------------------------------------------------------------------------
    // Constants

    /// Special index representing "all subresources" for transition commands.
    constexpr uint32_t AllSubresources = 0xffffffff;

    /// The maximum number of color attachments for a framebuffer.
    constexpr uint32_t MaxColorAttachments = 8;

    /// The maximum number of buffers a swapchain can contain.
    constexpr uint32_t MaxFrameCount = 3;

    /// Api name for the Null backend.
    /// Can be assigned to \ref InstanceDesc::api to initialize a Null backend.
    constexpr StringView Api_Null = "null";

    /// Api name for D3D12.
    /// Can be assigned to \ref InstanceDesc::api to initialize a D3D12 backend.
    constexpr StringView Api_D3D12 = "d3d12";

    // --------------------------------------------------------------------------------------------
    // Api interfaces

    class CopyCmdList;
    class CopyCmdQueue;
    class ComputeCmdList;
    class ComputeCmdQueue;
    class Device;
    class RenderCmdList;
    class RenderCmdQueue;
    class Instance;

    // --------------------------------------------------------------------------------------------
    // Device resources

    struct Adapter {};              ///< Physical adapters (video cards) attached to the system.
    struct Buffer {};               ///< Memory buffer that exists on a GPU heap.
    struct BufferView {};           ///< Read-only view of a buffer as a particular format and type.
    struct CmdAllocator {};         ///< Allocator for GPU commands placed into command lists.
    struct ComputePipeline {};      ///< Pipeline state for compute operations.
    struct ConstantBufferView {};   ///< A view of a buffer that contains constants.
    struct CpuFence {};             ///< A CPU-waitable fence that is signaled by GPU work.
    struct DescriptorTable {};      ///< Table of descriptors to bind to shader resource slots.
    struct Display {};              ///< Output device for an adapter.
    struct DisplayMode {};          ///< Output mode for a display.
    struct GpuFence {};             ///< A GPU-waitable fence that is signaled by GPU work.
    struct RenderPipeline {};       ///< Pipeline state for render operations.
    struct RenderTargetView {};     ///< A target texture view that can be rendered to.
    struct RootSignature {};        ///< Layout definition for shader input slots.
    struct RWBufferView {};         ///< Read-write view of a buffer as a particular format and type.
    struct RWTextureView {};        ///< Read-write view of a texture as a particular format and type.
    struct Sampler {};              ///< Definition of how to read a texel from a shader input.
    struct Shader {};               ///< Program that executes in a render or compute pipeline.
    struct SwapChain {};            ///< Series of framebuffers used to present graphics.
    struct Texture {};              ///< Series of pixels that live in a GPU heap.
    struct TextureView {};          ///< Read-only view of a texture as a particular format and type.
    struct VertexBufferFormat {};   ///< Definition of the format of a buffer of vertices.

    // --------------------------------------------------------------------------------------------
    // Formats

    /// Enumeration of supported buffer/texture formats.
    enum class Format : uint8_t
    {
        Invalid,

        // 8-bit formats
        R8Unorm,
        R8Snorm,
        R8Uint,
        R8Sint,
        A8Unorm,

        // 16-bit formats
        R16Unorm,
        R16Snorm,
        R16Uint,
        R16Sint,
        R16Float,
        RG8Unorm,
        RG8Snorm,
        RG8Uint,
        RG8Sint,

        // 16-bit packed formats
        BGRA4Unorm,

        // 32-bit formats
        R32Uint,
        R32Sint,
        R32Float,
        RG16Unorm,
        RG16Snorm,
        RG16Uint,
        RG16Sint,
        RG16Float,
        RGBA8Unorm,
        RGBA8Unorm_sRGB,
        RGBA8Snorm,
        RGBA8Uint,
        RGBA8Sint,
        BGRA8Unorm,
        BGRA8Unorm_sRGB,

        // 32-bit packed formats
        RGB10A2Unorm,
        RGB10A2Uint,
        RG11B10Float,
        RGB9E5SharedExp,

        // 64-bit formats
        RG32Uint,
        RG32Sint,
        RG32Float,
        RGBA16Unorm,
        RGBA16Snorm,
        RGBA16Uint,
        RGBA16Sint,
        RGBA16Float,

        // 96-bit formats
        RGB32Uint,
        RGB32Sint,
        RGB32Float,

        // 128-bit formats
        RGBA32Uint,
        RGBA32Sint,
        RGBA32Float,

        // Compressed BC formats
        BC1_RGBA,
        BC1_RGBA_sRGB,
        BC2_RGBA,
        BC2_RGBA_sRGB,
        BC3_RGBA,
        BC3_RGBA_sRGB,
        BC4_RUnorm,
        BC4_RSnorm,
        BC5_RGUnorm,
        BC5_RGSnorm,
        BC6H_RGBFloat,
        BC6H_RGBUfloat,
        BC7_RGBA,
        BC7_RGBA_sRGB,

        // Depth/stencil formats
        Depth16Unorm,
        Depth32Float,
        Depth24Unorm_Stencil8,
        Depth32Float_Stencil8,

        _Count,
    };

    /// Enumeration of the supported color spaces for swap chains and displays.
    enum class ColorSpace : uint8_t
    {
        sRGB,       ///< Standard RGB, BT.709, Gamma 2.2
        scRGB,      ///< Extended RGB, BT.709, Gamma 1.0
        HDR10_PQ,   ///< High-dynamic-range, BT.2020, SMPTE ST.2084 (Perceptual Quantization)

        _Count,
    };

    // --------------------------------------------------------------------------------------------
    // Api Types

    /// Normalization of OS results for specific render api results.
    enum class ApiResult : uint8_t
    {
        Success,        ///< The operation was successful.
        Failure,        ///< The operation failed for an unknown reason.
        DeviceLost,     ///< The device was lost and must be reinitialized.
        OutOfMemory,    ///< The device is out of memory and must be reinitialized.
        NotFound,       ///< The device was not found.
    };

    // --------------------------------------------------------------------------------------------
    // Instance Types

    /// PCI IDs for hardware vendors.
    enum class AdapterVendor : uint16_t
    {
        None        = 0x0000,
        Software    = 0x0001,
        AMD         = 0x1002,
        Intel       = 0x8086,
        nVidia      = 0x10de,
    };

    /// Information about an available adapater (video card).
    struct AdapterInfo
    {
        /// The PCI ID of the hardware vendor.
        AdapterVendor vendorId{ AdapterVendor::None };

        /// The PCI ID of the hardware device.
        uint16_t deviceId{ 0 };

        /// Locally unique value that identifies the adapter.
        uint64_t luid{ 0 };

        /// The number of bytes of dedicated video memory that are not shared with the CPU.
        uint64_t dedicatedVideoMemory{ 0 };

        /// The number of bytes of dedicated system memory that are not shared with the CPU.
        /// This memory is allocated from available system memory at boot time.
        uint64_t dedicatedSystemMemory{ 0 };

        /// The number of bytes of shared system memory. This is the maximum value of system
        /// memory that may be consumed by the adapter during operation. Any incidental memory
        /// consumed by the driver as it manages and uses video memory is additional.
        uint64_t sharedSystemMemory{ 0 };
    };

    /// Information about a display (output device).
    struct DisplayInfo
    {
        /// Constructs a display info structure.
        DisplayInfo(Allocator& a) noexcept : name(a) {}

        /// The name of the output device.
        String name;

        /// The position of the display.
        Vec2i pos{ 0, 0 };

        /// The size of the display.
        Vec2i size{ 0, 0 };

        /// The color space supported by this display.
        ColorSpace colorSpace{ ColorSpace::sRGB };
    };

    /// Information about a display mode.
    struct DisplayModeInfo
    {
        /// The resolution of the display mode.
        Vec2i resolution{ 0, 0 };

        /// The refresh rate of the display mode in hertz.
        float refreshRate{ 0 };

        /// Format supported for swap chains in this display mode.
        Format format{ Format::Invalid };
    };

    /// Descriptor for creating an instance.
    struct InstanceDesc
    {
        /// The allocator to use for all allocations in the lifetime of this instance.
        Allocator* allocator{ nullptr };

        /// Override for which API to use. By default, chooses the best one for the platform.
        /// Use the `he::rhi::Api_*` constants to set this.
        ///
        /// \note Not all values are valid for all platforms, so this value may get ignored.
        StringView api{};

        bool enableDebugCpu{ false };               ///< Enables cpu-side debugging features.
        bool enableDebugGpu{ false };               ///< Enables gpu-side debugging features.
        bool enableDebugBreakOnError{ false };      ///< Breaks to the debugger when there is an error. Requires `enableDebugCpu` to be set.
        bool enableDebugBreakOnWarning{ false };    ///< Breaks to the debugger when there is a warning. Requires `enableDebugCpu` to be set.
    };

    // --------------------------------------------------------------------------------------------
    // Buffer Types

    /// Enumeration of possible states for a buffer to be in.
    enum class BufferState : uint8_t
    {
        Common,             ///< Common state that allows a buffer to be used across queue types.

        // Read
        Indices,            ///< Reading as an index buffer.
        Vertices,           ///< Reading as a vertex buffer.
        Constants,          ///< Reading as a constants buffer.
        PixelShaderRead,    ///< Reading from a pixel shader.
        AnyShaderRead,      ///< Reading from any shader.
        CopySrc,            ///< Copying from the buffer to another resource.

        // Write
        CpuWrite,           ///< Writing on the CPU, assuming it is in a CPU-accessible heap.
        ShaderReadWrite,    ///< Reading and writing from any shader.
        CopyDst,            ///< Copying to the buffer from another resource.

        _Count,
    };

    /// Usage flags that describe the allowed ways a buffer can be used.
    enum class BufferUsage : uint32_t
    {
        None                = 0,
        Indices             = (1 << 0),     ///< Can be used as an index buffer.
        Vertices            = (1 << 1),     ///< Can be used as a vertex buffer.
        Constants           = (1 << 2),     ///< Can be used as a constants buffer.
        Structured          = (1 << 3),     ///< Can be used as a structured buffer.
        Typed               = (1 << 4),     ///< Can be used as a typed buffer.
        ShaderRead          = (1 << 5),     ///< Can be read from by shaders.
        ShaderReadWrite     = (1 << 6),     ///< Can be read from and written to by shaders.
        CopyDst             = (1 << 7),     ///< Can be the destination of a copy operation.
        CopySrc             = (1 << 8),     ///< Can be the source of a copy operation.
        All                 = (1 << 9) - 1, ///< Sets all the usage flags.

        /// Alias for a series of common copy source operations.
        Upload = Indices | Vertices | Constants | CopySrc,
    };
    HE_ENUM_FLAGS(BufferUsage);

    /// Type of a buffer view.
    enum class BufferViewType : uint8_t
    {
        Raw,
        Structured,
        Typed,
    };

    /// Type of a GPU heap.
    enum class HeapType : uint8_t
    {
        Default,    ///< No CPU access, GPU-read/write
        Upload,     ///< CPU-writable, GPU-readable, supports persistent mapping
        Readback,   ///< CPU-readable, GPU-writable, must be unmapped during a copy operation
        _Count,
    };

    /// Index type for a buffer used as an index buffer.
    enum class IndexType : uint8_t
    {
        Uint16,
        Uint32,
        _Count,
    };

    /// Descriptor for creating a buffer.
    struct BufferDesc
    {
        /// Type of heap this buffer lives on.
        HeapType heapType{ HeapType::Upload };

        /// Flags defining the usage allowed for this buffer.
        BufferUsage usage{ BufferUsage::Upload };

        /// The initial state of the buffer once created. Only when the heap type is set to
        /// \ref HeapType::Default will this value be honored. For example, a buffer on an
        /// \ref HeapType::Readback heap will always start in the \ref BufferState::CopyDst state.
        BufferState initialState{ BufferState::Common };

        /// Size (in bytes) of the buffer.
        uint32_t size{ 0 };

        /// Stride (in bytes) of each value within the buffer.
        uint32_t stride{ 1 };

        HE_RHI_NAME_MEMBER();
    };

    /// Descriptor for creating a buffer view.
    struct BufferViewDesc
    {
        /// The buffer to create the view for.
        const Buffer* buffer{ nullptr };

        /// Type of the view.
        BufferViewType type{ BufferViewType::Typed };

        /// Index to the first element of the view.
        uint32_t index{ 0 };

        /// Count of elements in the buffer. Only relevant when type is \ref BufferViewType::Structured.
        uint32_t count{ 0 };

        /// Format of the view. Only relevant when type is \ref BufferViewType::Typed.
        Format format{ Format::Invalid };
    };

    /// Descriptor for assigning a buffer as a constants buffer.
    struct ConstantBufferViewDesc
    {
        /// The buffer to use for the constant values.
        const Buffer* buffer{ nullptr };

        /// Offset (in bytes) into the buffer.
        uint32_t offset{ 0 };

        /// Size (in bytes) of the view into the buffer.
        uint32_t size{ 0 };
    };

    /// Descriptor that controls the region of a buffer copy operation.
    struct BufferCopy
    {
        /// Offset (in bytes) into the source buffer.
        uint32_t srcOffset{ 0 };

        /// Offset (in bytes) into the destination buffer.
        uint32_t dstOffset{ 0 };

        /// Number of bytes to be copied from the source to the destination.
        uint32_t size{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    // Texture Types

    /// Type of a texture, sometimes called the dimension.
    enum class TextureType : uint8_t
    {
        Unknown,        ///< The type is unknown.
        _1D,            ///< One dimensional texture.
        _1DArray,       ///< Array of one dimensional textures.
        _2D,            ///< Two dimensional texture.
        _2DArray,       ///< Array of two dimensional textures.
        _2DMS,          ///< Two dimensional, multisampled, texture.
        _2DMSArray,     ///< Array of two dimensional, multisampled, textures.
        _3D,            ///< Three dimensional texture.
        Cube,           ///< A cube map texture. A 2D texture with 6 layers, each the face of a cube.
        CubeArray,      ///< Array of cube map textures.
        _Count,
    };

    /// Enumeration of possible states for a texture to be in.
    enum class TextureState : uint8_t
    {
        Common,

        // Read
        PixelShaderRead,            ///< Reading from a pixel shader.
        PixelShaderDepthRead,       ///< Depth sampling, and reading, from a pixel shader.
        NonPixelShaderRead,         ///< Reading from any shader except a pixel shader.
        NonPixelShaderDepthRead,    ///< Depth sampling, and reading, from any shader except a pixel shader.
        AnyShaderRead,              ///< Reading from any shader.
        AnyShaderDepthRead,         ///< Depth sampling, and reading, from any shader.
        DepthRead,                  ///< Depth sampling but not writing.
        CopySrc,                    ///< Copying from the texture to another resource.
        Present,                    ///< Being presented to the display.
        ResolveSrc,                 ///< Source of a resolve operation.

        // Write
        ShaderReadWrite,            ///< Reading and writing from any shader.
        DepthWrite,                 ///< Written to as depth target, or being cleared.
        RenderTarget,               ///< Written to as the render target, or being cleared.
        CopyDst,                    ///< Copying to the texture from another resource.
        ResolveDst,                 ///< Destination of a resolve operation.

        _Count,
    };

    /// Usage flags that describe the allowed ways a texture can be used.
    enum class TextureUsage : uint32_t
    {
        None                = 0,
        ShaderRead          = (1 << 0),     ///< Can be read from by shaders.
        ShaderReadWrite     = (1 << 1),     ///< Can be read from and written to by shaders.
        RenderTarget        = (1 << 2),     ///< Can be written to as a render target.
        CopyDst             = (1 << 3),     ///< Can be the destination of a copy operation.
        CopySrc             = (1 << 4),     ///< Can be the source of a copy operation.
        All                 = (1 << 5) - 1, ///< Sets all the usage flags.
    };
    HE_ENUM_FLAGS(TextureUsage);

    /// Flags defining which parts of a render target view are cleared.
    enum class ClearFlag : uint32_t
    {
        None    = 0,            ///< Clear nothing.
        Depth   = (1 << 0),     ///< Clear the depth plane.
        Stencil = (1 << 1),     ///< Clear the stencil plane.
        All     = (1 << 2) - 1, ///< Clear everything.
    };
    HE_ENUM_FLAGS(ClearFlag);

    /// Enumeration of supported sample counts for textures. Defines the number of samples
    /// per pixel.
    enum class SampleCount : uint8_t
    {
        _1 = 1,     ///< One sample.
        _2 = 2,     ///< Two samples.
        _4 = 4,     ///< Four samples.
        _8 = 8,     ///< Eight samples.
        _16 = 16,   ///< Sixteen samples.
    };

    /// Descriptor for creating a texture.
    struct TextureDesc
    {
        /// The type of the texture.
        TextureType type{ TextureType::_2D };

        /// Allowed usage flags for the texture.
        TextureUsage usage{ TextureUsage::ShaderRead | TextureUsage::CopyDst };

        /// Pixel format of the texture's data.
        Format format{ Format::RGBA32Float };

        /// Size of the texture. The x/y/z components of the vector correspond to the
        /// width/height/depth of the texture. The depth (z) value is only used for 3D
        /// texture types.
        Vec3u size{ 0, 0, 1 };

        /// Number of layers in the texture array if this is a 1D or 2D array texture type.
        /// For cube map textures this value is the number of cube maps, not the number of faces.
        /// This value is ignored for 3D texture types.
        uint32_t layerCount{ 1 };

        /// Specifies the number of MIP levels.
        uint32_t mipCount{ 1 };

        /// Specifies the number of samples per pixel.
        SampleCount sampleCount{ SampleCount::_1 };

        /// The initial state of the texture once created.
        TextureState initialState{ TextureState::CopyDst };

        /// Color for which color clear operations are most optimal. This should be set to the
        /// value that will be used most often to clear the texture. This is only relevant for
        /// textures with the \ref TextureUsage::RenderTarget usage flag set.
        Vec4f optimizedClearColor{ 0, 0, 0, 0 };

        /// Value for which stencil clear operations are most optimal. This should be set to the
        /// value that will be used most often to clear the texture. This is only relevant for
        /// textures with the \ref TextureUsage::RenderTarget usage flag set, and that use a
        /// depth/stencil format.
        float optimizedClearDepth{ 0 };

        /// Value for which stencil clear operations are most optimal. This should be set to the
        /// value that will be used most often to clear the texture. This is only relevant for
        /// textures with the \ref TextureUsage::RenderTarget usage flag set, and that use a
        /// depth/stencil format.
        uint8_t optimizedClearStencil{ 0 };

        HE_RHI_NAME_MEMBER();
    };

    /// Descriptor for creating a texture view.
    struct TextureViewDesc
    {
        /// The texture to create the view for.
        const Texture* texture{ nullptr };

        /// The type of the view. Setting to \ref TextureType::Unknown (default) will use the
        /// source texture's type.
        TextureType type{ TextureType::Unknown };

        /// The pixel format of the view. Setting to \ref Format::Invalid (default) will use the
        /// source texture's format.
        Format format{ Format::Invalid };

        /// Index of the first mip to include in the view. The valid range is
        /// `[0, texture.mipLevels)`
        uint32_t mipIndex{ 0 };

        /// Number of mip levels to include in the view. A value of -1 means all remaining levels
        /// after `mipIndex`.
        uint32_t mipCount{ 1 };

        /// Index of the first layer in the texture array to include in the view. The valid range
        /// is `[0, texture.layerCount)`
        uint32_t layerIndex{ 0 };

        /// Number of layers in the texture array to include in the view.
        uint32_t layerCount{ 0 };

        /// Controls if depth values are read-only. Only relevant when creating a
        /// Render Target View, and the format is a depth/stencil format.
        bool readOnlyDepth{ false };

        /// Controls if stencil values are read-only. Only relevant when creating a Render Target View, and the format is a depth/stencil format.
        bool readOnlyStencil{ false };
    };

    /// Descriptor that controls the region of a texture copy operation.
    struct TextureCopy
    {
        /// The source mip level to copy from.
        uint32_t srcMip{ 0 };

        /// The source layer in the texture array to copy from. If the source texture
        /// is not an array, then leave this value as zero (default).
        uint32_t srcLayer{ 0 };

        /// Offset into the source texture to copy from.
        Vec3u srcOffset{ 0, 0, 0 };

        /// The destination mip level to copy to.
        uint32_t dstMip{ 0 };

        /// The destination layer in the texture array to copy to. If the destination texture
        /// is not an array, then leave this value as zero (default).
        uint32_t dstLayer{ 0 };

        /// Offset into the destination texture to copy to.
        Vec3u dstOffset{ 0, 0, 0 };

        /// Number of texels to copy from the source to the destination.
        /// The x/y/z components of the vector represent the width/height/depth of the region.
        Vec3u srcSize{ 0, 0, 0 };
    };

    /// Descriptor that controls the region of buffer <-> texture copy operation.
    struct BufferTextureCopy
    {
        /// Offset (in bytes) in the buffer to copy to or from.
        uint32_t bufferOffset{ 0 };

        /// The row pitch, width, or phsical size (in bytes) of the data. This must be a multiple
        /// of \ref DeviceInfo::uploadDataPitchAlignment and must be greater than or equal to
        /// the size of the data within a row.
        uint32_t bufferRowPitch{ 0 };

        /// The mip level of the texture to copy to or from.
        uint32_t textureMip{ 0 };

        /// The layer in the texture array to copy to or from. If the texture is not an array,
        /// then leave this value as zero (default).
        uint32_t textureLayer{ 0 };

        /// Offset into the texture to copy to or from.
        Vec3u textureOffset{ 0, 0, 0 };

        /// Number of texels in the texture to copy to or from.
        /// The x/y/z components of the vector represent the width/height/depth of the region.
        Vec3u textureSize{ 0, 0, 0 };
    };

    // --------------------------------------------------------------------------------------------
    // Vertex Buffer Format Types

    /// Identifies the type of data contained in a vertex input slot.
    enum class StepRate : uint8_t
    {
        PerVertex,      ///< Input data is per-vertex data.
        PerInstance,    ///< Input data is per-instance data.
    };

    /// Descriptor of an attribute in a vertex buffer format.
    struct VertexAttributeDesc
    {
        /// The semantic name of the attribute in the shader. For example, if the semantic
        /// `TEXCOORD3` is used in your shader the name here would be `"TEXCOORD"`.
        const char* semanticName{ "ATTR" };

        /// The semantic index of the attribute in the shader. For example, if the semantic
        /// `TEXCOORD3` is used in your shader the name here would be `3`.
        uint32_t semanticIndex{ 0 };

        /// Format of the element data in this attribute.
        Format format{ Format::Invalid };

        /// Offset (in bytes) to this element from the start of the vertex.
        uint32_t offset{ 0 };
    };

    /// Descriptor of a vertex buffer format.
    struct VertexBufferFormatDesc
    {
        /// The size (in bytes) of each vertex entry.
        uint32_t stride{ 0 };

        /// The type of data contained in a vertex input slot.
        StepRate stepRate{ StepRate::PerVertex };

        /// Number of attributes in an element of the vertex buffer.
        uint32_t attributeCount{ 0 };

        /// Array of attributes in an element of the vertex buffer.
        const VertexAttributeDesc* attributes{ nullptr };
    };

    // --------------------------------------------------------------------------------------------
    // Swap Chain Types

    /// Scaling behavior if the size of the back buffer is not equal to the target output.
    enum class SwapChainScaling : uint8_t
    {
        None,               ///< No scaling happens. The top-left edges are aligned with the presentation target.
        Stretch,            ///< Stretch to fit the presentation target size.
        AspectRatioStretch, ///< Stretch to fit the presentation target size, while preserving the aspect ratio.
        _Count,
    };

    /// Format of a swap chain.
    struct SwapChainFormat
    {
        /// Pixel format of the swap chain's backbuffer.
        Format format{ Format::Invalid };

        /// Color space utilized by the backbuffer.
        ColorSpace colorSpace{ ColorSpace::sRGB };
    };

    /// Descriptor for creating a swap chain.
    struct SwapChainDesc
    {
        /// Native handle of the platform view the swap chain is for.
        void* nativeViewHandle{ nullptr };

        /// The number of buffers in the swap chain.
        uint32_t bufferCount{ MaxFrameCount };

        /// Enable vertical sync which will synchronize presentation with monitor's refresh rate.
        bool enableVSync{ true };

        /// Scaling behavior when the swap chain does not match the size of the presentation target.
        SwapChainScaling scalingMode{ SwapChainScaling::Stretch };

        /// Format of the swap chain buffers.
        SwapChainFormat format{};

        /// Size of the swap chain buffers.
        Vec2i size{ 0, 0 };
    };

    /// Present target that can be acquired to render to the swap chain.
    struct PresentTarget
    {
        /// The texture of the active backbuffer.
        const Texture* texture{ nullptr };

        /// The render target view of the active backbuffer.
        const RenderTargetView* renderTargetView{ nullptr };
    };

    // --------------------------------------------------------------------------------------------
    // Device Types

    /// Descriptor for creating a device.
    struct DeviceDesc
    {
        /// Locally unique identifier of the adapter to use. Settings this to zero (default)
        /// attempts to choose the "best" adapter.
        uint64_t luid{ 0 };

        /// The maximum number of present operations that can be queued for rendering before the
        /// CPU must stall.
        uint8_t maxFrameLatency{ 3 };

        /// D3D12 specific options.
        struct
        {
            /// Sizes of the CPU descriptor heaps to be allocated. These are the max number of
            /// descriptors that can be allocated on each heap type. Increasing these values will
            /// allow the creation of additional resources, at the cost of more memory.
            struct
            {
                /// Number of buffer descriptors to allocate.
                uint16_t buffer{ 4096 };

                /// Number of sampler descriptors to allocate.
                uint16_t sampler{ 64 };

                /// Number of render target descriptors to allocate.
                uint16_t renderTarget{ 1024 };

                /// Number of depth stencil descriptors to allocate.
                uint16_t depthStencil{ 64 };
            } cpuDescriptorHeapSizes{};

            /// Sizes of the GPU descriptor heaps to be allocated. These are the max number of
            /// descriptors that can be allocated on each heap type. Increasing these values will
            /// allow the creation of additional resources, at the cost of more memory.
            struct
            {
                /// Number of buffer descriptors to allocate.
                uint16_t buffer{ 4096 };

                /// Number of sampler descriptors to allocate.
                uint16_t sampler{ 64 };
            } gpuDescriptorHeapSizes{};
        } d3d12;

        HE_RHI_NAME_MEMBER();
    };

    /// Type of a command list.
    enum class CmdListType : uint8_t
    {
        Copy,       ///< List that can contain copy commands.
        Compute,    ///< List that can contain copy and compute commands.
        Render,     ///< List that can contain copy, compute, and render commands.
        _Count,
    };

    /// Descriptor for creating a command allocator.
    struct CmdAllocatorDesc
    {
        /// Types of command list commands that can be allocated by this allocator.
        CmdListType type{ CmdListType::Render };

        HE_RHI_NAME_MEMBER();
    };

    /// Descriptor for creating a command list.
    struct CmdListDesc
    {
        /// Allocator used to create and initialize the command list.
        CmdAllocator* alloc{ nullptr };

        HE_RHI_NAME_MEMBER();
    };

    /// Descriptor for creating a CPU fence.
    struct CpuFenceDesc
    {
    };

    /// Descriptor for creating a GPU fence.
    struct GpuFenceDesc
    {
    };

    // --------------------------------------------------------------------------------------------
    // Shader Types

    /// Enumeration of supported shader stages.
    enum class ShaderStage : uint8_t
    {
        All,
        Vertex,
        Hull,
        Domain,
        Geometry,
        Pixel,
        Compute,
        _Count,
    };

    /// Enumeration of supported shader models
    enum class ShaderModel
    {
        None,
        Sm_6_0,     ///< Shader Model 6.0, DX 12, GCN 1+, Kepler+, with WDDM 2.1.
        Sm_6_1,     ///< Shader Model 6.1, DX 12, GCN 1+, Kepler+, with WDDM 2.3.
        Sm_6_2,     ///< Shader Model 6.2, DX 12, GCN 1+, Kepler+, with WDDM 2.4.
        Sm_6_3,     ///< Shader Model 6.3, DX 12, GCN 1+, Kepler+, with WDDM 2.5.
        Sm_6_4,     ///< Shader Model 6.4, DX 12, GCN 1+, Kepler+, Skylake+, with WDDM 2.6.
        Sm_6_5,     ///< Shader Model 6.5, DX 12, GCN 1+, Kepler+, Skylake+, with WDDM 2.7.
        Sm_6_6,     ///< Shader Model 6.6, DX 12, GCN 1+, Kepler+, Skylake+, with WDDM 2.9.
        Sm_6_7,     ///< Shader Model 6.7, DX 12
        Sv_1_3,     ///< Spir-V 1.3, Vulkan 1.1
        Sv_1_4,     ///< Spir-V 1.4, Vulkan 1.1
        Sv_1_5,     ///< Spir-V 1.5, Vulkan 1.2
        _Count,
    };

    /// Descriptor for creating a shader.
    struct ShaderDesc
    {
        /// Stage the shader is to be run on.
        ShaderStage stage{ ShaderStage::Pixel };

        /// Compiled shader code. The actual contents of this buffer vary by backend.
        ///
        /// - D3D12: Byte code from D3DCompile, or DXIL from DXC
        ///     * dxil.dll is required to ship next to your exe if you want to use DXIL as input
        /// - Vulkan: SPIR-V byte code
        const void* code{ nullptr };

        /// Size (in bytes) of the code buffer.
        uint32_t codeSize{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    // Pipeline Types

    /// Descriptor for creating a compute pipeline.
    struct ComputePipelineDesc
    {
        /// The root signature that describes the layout.
        const RootSignature* rootSignature{ nullptr };

        /// The shader instance to bind.
        const Shader* shader{ nullptr };

        HE_RHI_NAME_MEMBER();
    };

    /// Enumeration of supported blend factors, which modulate values for the pixel shader
    /// and render target.
    enum class BlendFactor : uint8_t
    {
        Zero,           ///< (0, 0, 0, 0)
        One,            ///< (1, 1, 1, 1)
        SrcAlpha,       ///< (As, As, As, As)
        InvSrcAlpha,    ///< (1-As, 1-As, 1-As, 1-As)
        SrcColor,       ///< (Rs, Gs, Bs, As)
        InvSrcColor,    ///< (1-Rs, 1-Gs, 1-Bs, 1-As)
        DstAlpha,       ///< (Ad, Ad, Ad, Ad)
        InvDstAlpha,    ///< (1-Ad, 1-Ad, 1-Ad, 1-Ad)
        DstColor,       ///< (Rd, Gd, Bd, Ad)
        InvDstColor,    ///< (1-Rd, 1-Gd, 1-Bd, 1-Ad)
        SrcAlphaSat,    ///< (f, f, f, 1) where f = min(As, 1-Ad) clamped <= 1
        BlendFactor,    ///< (Cr, Cg, Cb, Ca) where C is specified with \ref RenderCmdList::SetBlendColor
        InvBlendFactor, ///< (1-Cr, 1-Cg, 1-Cb, 1-Ca) where C is specified with \ref RenderCmdList::SetBlendColor
        Src1Alpha,      ///< (As, As, As, As), supports dual-source color blending
        InvSrc1Alpha,   ///< (1-As, 1-As, 1-As, 1-As), supports dual-source color blending
        Src1Color,      ///< (Rs, Gs, Bs, As), supports dual-source color blending
        InvSrc1Color,   ///< (1-Rs, 1-Gs, 1-Bs, 1-As), supports dual-source color blending
        _Count,
    };

    /// Enumeration of supported blending operations.
    enum class BlendOp : uint8_t
    {
        Add,            ///< source + destination
        Subtract,       ///< source - destination
        RevSubtract,    ///< destination - source
        Min,            ///< Min(source, destination)
        Max,            ///< Max(source, destination)
        _Count,
    };

    /// Identifies which components of each pixel of a render target are writable during blending.
    enum class ColorWriteMask : uint8_t
    {
        None        = 0,
        Red         = (1 << 0),
        Green       = (1 << 1),
        Blue        = (1 << 2),
        Alpha       = (1 << 3),
        All         = Red | Green | Blue | Alpha
    };
    HE_ENUM_FLAGS(ColorWriteMask);

    /// Specifies triangles facing a particular direction are not drawn.
    enum class CullMode : uint8_t
    {
        None,   ///< Always draw all triangles.
        Front,  ///< Do not draw triangles that are front-facing.
        Back,   ///< Do not draw triangles that are back-facing.
        _Count,
    };

    /// Specifies how the pipeline interprets geometry or hull shader input primitives.
    enum class PrimitiveType : uint8_t
    {
        PointList,  ///< Interpret the input data as a list of points.
        LineList,   ///< Interpret the input data as a list of lines.
        LineStrip,  ///< Interpret the input data as a line strip.
        TriList,    ///< Interpret the input data as a list of triangles.
        TriStrip,   ///< Interpret the input data as a triangle strip.
        _Count,
    };

    /// Identifies the stencil operations that can be performed during depth-stencil testing.
    enum class StencilOp : uint8_t
    {
        Keep,       ///< Keep the existing stencil data.
        Zero,       ///< Set the stencil data to 0.
        Replace,    ///< Set the stencil data to the reference value set by \ref RenderCmdList::SetStencilRef
        IncClamp,   ///< Increment the stencil value by 1, and clamp the result.
        DecClamp,   ///< Decrement the stencil value by 1, and clamp the result.
        Invert,     ///< Invert the stencil data.
        IncWrap,    ///< Increment the stencil value by 1, and wrap the result if necessary.
        DecWrap,    ///< Decrement the stencil value by 1, and wrap the result if necessary.
        _Count,
    };

    /// Enumeration of supported comparison functions for depth/stencil/sampler comparisons.
    enum class ComparisonFunc : uint8_t
    {
        Never,          ///< Never pass the comparison.
        Less,           ///< source < destination
        Equal,          ///< source == destination
        LessEqual,      ///< source <= destination
        Greater,        ///< source > destination
        NotEqual,       ///< source != destination
        GreaterEqual,   ///< source >= destination
        Always,         ///< Always pass the comparison.
        _Count,
    };

    /// Enumeration of supported fill modes to use when rendering triangles.
    enum class FillMode : uint8_t
    {
        Solid,      ///< Fill the triangles formed by the vertices. Adjacent vertices are not drawn.
        Wireframe,  ///< Draw lines connecting the vertices. Adjacent vertices are not drawn.
        _Count,
    };

    /// Descriptor of a blend target in the blend state of a render pipeline.
    struct BlendTargetDesc
    {
        /// When true blending is enabled.
        bool enable{ false };

        /// Blend factor for the source RGB (pixel shader output)
        BlendFactor srcRgb{ BlendFactor::One };

        /// Blend factor for the destination RGB (render target content)
        BlendFactor dstRgb{ BlendFactor::Zero };

        /// Operation to combine the RGB source and destination blend factors.
        BlendOp opRgb{ BlendOp::Add };

        /// Blend factor for the source Alpha (pixel shader output)
        BlendFactor srcAlpha{ BlendFactor::One };

        /// Blend factor for the destination Alpha (render target content)
        BlendFactor dstAlpha{ BlendFactor::Zero };

        /// Operation to combine the Alpha source and destination blend factors.
        BlendOp opAlpha{ BlendOp::Add };

        /// Mask of which color channels can be written during the blend.
        ColorWriteMask writeMask{ ColorWriteMask::All };
    };

    /// Descriptor of the blend state for a render pipeline.
    struct BlendDesc
    {
        /// Enables using alpha-to-coverage as a multisampling technique when setting a pixel to
        /// a render target.
        bool alphaToCoverageEnable{ false };

        /// Enables independent blending in simultaneous render targets. If set to false (default),
        /// only the first render target is used.
        bool independentBlendEnable{ false };

        /// Describes how to blend to each render target in the pipeline. Only the first
        /// \ref TargetsDesc::renderTargetCount entries are considered.
        BlendTargetDesc targets[MaxColorAttachments];
    };

    /// Descriptor of the rasterizer state for a render pipeline.
    struct RasterDesc
    {
        /// The fill mode to use when rendering triangles.
        FillMode fillMode{ FillMode::Solid };

        /// The culling mode to use when rendering triangles.
        CullMode cullMode{ CullMode::Back };

        /// Depth value added to a given pixel.
        int32_t depthBias{ 0 };

        /// Scalar on a given pixel's slope.
        float slopeScaledDepthBias{ 0.0f };

        /// Maximum depth bias of a pixel.
        float depthBiasClamp{ 0.0f };

        /// When false (default) clamping of `z` values is disabled. When disabled, improper depth
        /// ordering at the pixel level might result but can also simplify stencil shadows.
        bool depthClamp{ false };

        /// When true (default) a triangle will be considered front-facing if its vertices are
        /// counter-clockwise on the render target and considered back-facing if they are
        /// clockwise. When false the direction is reversed.
        bool frontCounterClockwise{ true };
    };

    /// Descriptor of the depth state for a render pipeline.
    struct DepthDesc
    {
        /// When true (default) enables depth testing.
        bool testEnable{ true };

        /// When true (default) enables writing to the depth/stencil buffer.
        bool writeEnable{ true };

        /// Function to compare depth data against existing depth data.
        ComparisonFunc func{ ComparisonFunc::Greater };
    };

    /// Descriptor of stencil operations that can be performed based on the results of stencil test.
    struct StencilFaceDesc
    {
        /// The stencil operation to perform when stencil testing fails.
        StencilOp failOp{ StencilOp::Keep };

        /// The stencil operation to perform when stencil testing passes and depth testing fails.
        StencilOp depthFailOp{ StencilOp::Keep };

        /// The stencil operation to perform when stencil testing and depth testing both pass.
        StencilOp passOp{ StencilOp::Keep };

        /// Function to compare stencil data against existing stencil data.
        ComparisonFunc func{ ComparisonFunc::Always };
    };

    /// Descriptor of the stencil state for a render pipeline.
    struct StencilDesc
    {
        /// When true (default) enables stencil testing.
        bool enable{ false };

        /// Mask of which portion of the stencil buffer can be read.
        uint8_t readMask{ 0xff };

        /// Mask of which portion of the stencil buffer can be written.
        uint8_t writeMask{ 0xff };

        /// How to use the results of the depth test and the stencil test for pixels whose surface
        /// normal is facing towards the camera.
        StencilFaceDesc frontFace{};

        /// How to use the results of the depth test and the stencil test for pixels whose surface
        /// normal is facing away from the camera.
        StencilFaceDesc backFace{};
    };

    /// Descriptor of render target view and depth/stencil buffer formats for a render pipeline.
    struct TargetsDesc
    {
        /// Number of render target views this pipeline renders to.
        uint32_t renderTargetCount{ 0 };

        /// Formats of the render target views this pipeline renders to. The first `renderTargetCount`
        /// entries are considered.
        Format renderTargetFormats[MaxColorAttachments]{};

        /// Format of the depth/stencil buffer in this render pipeline.
        Format depthStencilFormat{ Format::Invalid };

        /// Specifies the number of samples per pixel.
        SampleCount sampleCount = SampleCount::_1;
    };

    /// Descriptor for creating a render pipeline.
    struct RenderPipelineDesc
    {
        /// The root signature that describes the layout of the pipeline.
        const RootSignature* rootSignature{ nullptr };

        /// The shader to execute for the vertex shader stage. This shader is required.
        const Shader* vertexShader{ nullptr };

        /// The shader to execute for the hull shader stage.
        const Shader* hullShader{ nullptr };

        /// The shader to execute for the domain shader stage.
        const Shader* domainShader{ nullptr };

        /// The shader to execute for the geometry shader stage.
        const Shader* geometryShader{ nullptr };

        /// The shader to execute for the pixel shader stage.
        const Shader* pixelShader{ nullptr };

        /// Number of vertex buffers in the render pipeline.
        uint32_t vertexBufferCount{ 0 };

        /// Array of vertex buffer formats for each vertex buffer in the render pipeline.
        const VertexBufferFormat* const* vertexBufferFormats{ nullptr };

        /// Type of the input primitives sent in the input assembler stage.
        PrimitiveType primitiveType{ PrimitiveType::TriList };

        /// The blend state of the pipeline.
        BlendDesc blend{};

        /// The raster state of the pipeline.
        RasterDesc raster{};

        /// The depth state of the pipeline.
        DepthDesc depth{};

        /// The stencil state of the pipeline.
        StencilDesc stencil{};

        /// Targets rendered to by the pipeline.
        TargetsDesc targets{};

        HE_RHI_NAME_MEMBER();
    };

    /// Operation to perform on an attachment at the beginning of a render pass.
    enum class LoadOp : uint8_t
    {
        Load,       ///< Preserve the previous contents of the attachment.
        Clear,      ///< Clear the contents of the attachment.
        DontCare,   ///< Previous contents of the attachment need not be preserved; the contents of the attachment are undefined.
    };

    /// Operation to perform on an attachment at the end of a render pass.
    enum class StoreOp : uint8_t
    {
        Store,              ///< Contents generated during the render pass are written to memory.
        DontCare,           ///< Contents are not needed after rendering, and may be discarded; the contents of the attachment are undefined.
        Resolve,            ///< Resolve the attachment view to the specified resolved view.
        StoreAndResolve,    ///< Performs both the Store and Resolve operations.
    };

    /// Actions to perform at load and store time for an attachment in a render pass.
    template <typename ClearType>
    struct RenderPassAction
    {
        /// The operation to perform at load time (render pass begin).
        LoadOp load{ LoadOp::Load };

        /// The operation to perform at store time (render pass end).
        StoreOp store{ StoreOp::Store };

        /// The value to clear the attachment to when using the Clear load and/or store operation.
        ClearType clearValue{};

        /// The view to resolve to when using the Resolve or StoreAndResolve store operation.
        const RenderTargetView* resolveToView{ nullptr };
    };

    /// Base for an attachment in a render pass.
    struct Attachment
    {
        /// The render target view that is attached.
        const RenderTargetView* view{ nullptr };

        /// The current state of the render target view's texture. The texture will be
        /// transitioned to the \ref TextureState::Render or \ref TextureState::DepthWrite state
        /// when the render pass beings, then transitioned back into this state when it ends.
        TextureState state{ TextureState::Common };
    };

    /// A color texture attachment for a render pass.
    struct ColorAttachment : Attachment
    {
        /// The action to perform for the attachment at begin/end of the render pass.
        RenderPassAction<Vec4f> action{};
    };

    /// A depth/stencil texture attachment for a render pass.
    struct DepthStencilAttachment : Attachment
    {
        /// The action to perform for the depth attachment at begin/end of the render pass.
        RenderPassAction<float> depthAction{};

        /// The action to perform for the stencil attachment at begin/end of the render pass.
        RenderPassAction<uint8_t> stencilAction{};
    };

    /// Descriptor for starting a render pass.
    struct RenderPassDesc
    {
        /// Number of color attachments for this pass.
        uint32_t colorAttachmentCount{ 0 };

        /// Array of color attachments for this pass.
        const ColorAttachment* colorAttachments{ nullptr };

        /// An optional depth/stencil attachment for this pass.
        const DepthStencilAttachment* depthStencilAttachment{ nullptr };
    };

    // --------------------------------------------------------------------------------------------
    // Descriptor Table Types

    /// Enumeration of supported types of descriptor ranges.
    enum class DescriptorRangeType : uint8_t
    {
        StructuredBuffer,   ///< A read-only buffer which holds a series of type T that is a structure.
        TypedBuffer,        ///< A read-only buffer which holds a series of a format type.
        Texture,            ///< A read-only N-dimensional texture which passes through filter stages.
        RWStructuredBuffer, ///< A read-write buffer which holds a series of type T that is a structure.
        RWTypedBuffer,      ///< A read-write buffer which holds a series of a format type.
        RWTexture,          ///< A read-write N-dimensional texture which passes through filter stages.
        ConstantBuffer,     ///< A read-only constant buffer which is a structure of constant values.
        Sampler,            ///< An object used for sampling texels from a texture.
        _Count,
    };

    /// A typed range in a descriptor table.
    struct DescriptorRange
    {
        /// Constant value that signals an unbounded range when used the `count` on the root signature.
        static constexpr uint32_t Unbounded{ 0xffffffff };

        /// The type of entries the descriptor range contains.
        DescriptorRangeType type{ DescriptorRangeType::Texture };

        /// The base shader register in the range.
        /// For example, for a shader resource using `: register(t3)` this value is `3`.
        uint32_t baseRegister{ 0 };

        /// The shader register space, typically zero (default).
        /// For example, for a shader resource using `: register(t3, space5)` this value is `5`.
        uint32_t registerSpace{ 0 };

        /// The number of descriptors in the range. Set to \ref Unbounded to signal that this
        /// range is unbounded.
        ///
        /// \note Only the last range in a table can be unbounded.
        uint32_t count{ 0 };
    };

    /// Descriptor for creating a descriptor table.
    struct DescriptorTableDesc
    {
        /// Number of ranges in this table.
        uint32_t rangeCount{ 0 };

        /// Array of ranges.
        const DescriptorRange* ranges{ nullptr };
    };

    // --------------------------------------------------------------------------------------------
    // Sampler Types

    /// Enumeration of supported magnification and minification sampler filters.
    enum class Filter : uint8_t
    {
        Nearest,    ///< Min/Mag uses nearest texel to desired pixel. Mip uses nearest-point mipmap filters. Raster uses nearest mip color.
        Linear,     ///< Min/Mag use bilinear interpolation. Mip uses trilinear mipmap interpolation. Raster linearly interpolates mip colors.
        _Count,
    };

    /// Enumeration of supported filter reduction operations for samplers.
    enum class FilterReduce : uint8_t
    {
        Standard,
        Compare,
        Min,
        Max,
        _Count,
    };

    /// Enumeration of supported addressing modes for samplers.
    enum class AddressMode : uint8_t
    {
        Border,     ///< (u, v) outside the range [0, 1] are set to the border color.
        Clamp,      ///< (u, v) outside the range [0, 1] are set to the texture color at 0 or 1, respectively.
        Mirror,     ///< Flip the texture at every (u, v) integer junction.
        MirrorOnce, ///< Take absolutely value of (u, v) and then clamp to the maximum value.
        Wrap,       ///< Tile at every (u, v) integer junction.
        _Count,
    };

    /// Border color to use when the addressing mode is \ref AddressMode::Border
    enum class BorderColor : uint8_t
    {
        Black,              ///< Black, with the alpha component fully opaque.
        TransparentBlack,   ///< Black, with the alpha component fully transparent.
        White,              ///< White, with the alpha component fully opaque.
        _Count,
    };

    struct SamplerDesc
    {
        /// The minification filter to use.
        /// When \ref maxAnisotropy is greater than one, this value is treated as Linear.
        Filter minFilter{ Filter::Linear };

        /// The magnification filter to use.
        /// When \ref maxAnisotropy is greater than one, this value is treated as Linear.
        Filter magFilter{ Filter::Linear };

        /// The mipmap filter to use.
        /// When \ref maxAnisotropy is greater than one, this value is treated as Linear.
        Filter mipFilter{ Filter::Linear };

        /// The filter reduction mode.
        /// \note This is not supported on all backends.
        FilterReduce filterReduce{ FilterReduce::Standard };

        /// The addressing mode for resolving `u` texture coordinates outside the `[0, 1]` range.
        AddressMode addressU{ AddressMode::Clamp };

        /// The addressing mode for resolving `v` texture coordinates outside the `[0, 1]` range.
        AddressMode addressV{ AddressMode::Clamp };

        /// The addressing mode for resolving `w` texture coordinates outside the `[0, 1]` range.
        AddressMode addressW{ AddressMode::Clamp };

        /// Clamping value used for anisotropic filtering. Must be in the range `[1, 16]`. Any value
        /// larger than `1` enables anisotropic filtering.
        uint32_t maxAnisotropy{ 1 };

        /// Function to compare sampled data against existing sampled data.
        ComparisonFunc comparisonFunc{ ComparisonFunc::Never };

        /// The border color to use if any addressing mode is set to \ref AddressMode::Border
        BorderColor borderColor{ BorderColor::White };

        /// Lower end of the mipmap range to clamp access to.
        float minLOD{ 0.0f };

        /// Upper end of the mipmap range to clamp access to. This value must be greater than or
        /// equal to \ref minLOD.
        float maxLOD{ 16.0f };

        /// Offset from the calculated mipmap level to the desired mip level. For example, if
        /// mipmap level 3 is chosen, and `lodBias` is `2` then the texture is sampled at mipmap
        /// level 5.
        float lodBias{ 0.0f };

        HE_RHI_NAME_MEMBER();
    };

    /// Descriptor of a static sampler that can be placed in a root signature.
    struct StaticSamplerDesc : SamplerDesc
    {
        /// The shader stage this sampler is visible to.
        ShaderStage stage{ ShaderStage::Pixel };

        /// The base shader register in the range.
        /// For example, for a shader resource using `: register(t3)` this value is `3`.
        uint32_t baseRegister{ 0 };

        /// The shader register space, typically zero (default).
        /// For example, for a shader resource using `: register(t3, space5)` this value is `5`.
        uint32_t registerSpace{ 0 };
    };

    // --------------------------------------------------------------------------------------------
    // Root Signature Types

    /// Type of a slot in a root signature.
    enum class SlotType : uint8_t
    {
        DescriptorTable,    ///< A table of descriptor entries, costs 1 DWORD and 2 indirections
        ConstantBuffer,     ///< A constant buffer descriptor, costs 2 DWORDs and 1 indirection
        ConstantValues,     ///< A series of constant values, each costs 1 DWORD and 0 indirections
    };

    /// Descriptor for a constant buffer placed in the root signature.
    struct RootConstantBufferDesc
    {
        /// The base shader register in the range.
        /// For example, for a shader resource using `: register(t3)` this value is `3`.
        uint32_t baseRegister{ 0 };

        /// The shader register space, typically zero (default).
        /// For example, for a shader resource using `: register(t3, space5)` this value is `5`.
        uint32_t registerSpace{ 0 };
    };

    /// Descriptor for a series of constant values placed in the root signature.
    struct RootConstantValuesDesc
    {
        /// The base shader register in the range.
        /// For example, for a shader resource using `: register(t3)` this value is `3`.
        uint32_t baseRegister{ 0 };

        /// The shader register space, typically zero (default).
        /// For example, for a shader resource using `: register(t3, space5)` this value is `5`.
        uint32_t registerSpace{ 0 };

        /// Number of 32-bit constant values
        uint32_t num32BitValues{ 0 };
    };

    /// Descriptor of a slot in the root signature. It contains a union of descriptions.
    /// Which description is valid must match the given type.
    struct SlotDesc
    {
        /// The type of this slot.
        SlotType type{ SlotType::DescriptorTable };

        /// The shader stage this slot is visible to.
        ShaderStage stage{ ShaderStage::Pixel };

        union
        {
            /// The descriptor table configuration if \ref type is \ref SlotType::DescriptorTable.
            DescriptorTableDesc descriptorTable{};

            /// The constant buffer configuration if \ref type is \ref SlotType::ConstantBuffer.
            RootConstantBufferDesc constantBuffer;

            /// The constant value configuration if \ref type is \ref SlotType::ConstantValue.
            RootConstantValuesDesc constantValues;
        };
    };

    /// Descriptor for creating a root signature.
    struct RootSignatureDesc
    {
        /// Number of slots.
        uint32_t slotCount{ 0 };

        /// Array of slots.
        const SlotDesc* slots{ nullptr };

        /// Number of static samplers.
        uint32_t staticSamplerCount{ 0 };

        /// Array of static samplers.
        const StaticSamplerDesc* staticSamplers{ nullptr };

        /// When true (default) the input assembler stage is enabled. Disabling the input
        /// assembler can result in one root argument space being saved on some hardware.
        /// Disable this if the Input Assembler is not required, though the optimization is minor.
        bool inputAssembler{ true };

        HE_RHI_NAME_MEMBER();
    };

    // --------------------------------------------------------------------------------------------
    // Miscellaneous

    /// Describes the dimensions of a viewport.
    struct Viewport
    {
        float x;        ///< X posiiton of the left side of the viewport
        float y;        ///< Y position of the left side of the viewport
        float width;    ///< Width of the viewport
        float height;   ///< Height of the viewport
        float minZ;     ///< Minimum depth of the viewport. Must be in the range [0, 1]
        float maxZ;     ///< Maximum depth of the viewport. Must be in the range [0, 1]
    };

    /// Descriptor for drawing non-indexed, instanced primitives.
    struct DrawDesc
    {
        /// Number of vertices to draw.
        uint32_t vertexCount;

        /// Number of instances to draw.
        uint32_t instanceCount;

        //// Index of the first vertex to draw.
        uint32_t vertexStart;

        /// A values added to each index before per-instance data from a vertex buffer.
        uint32_t baseInstance;
    };

    /// Descriptor for drawing indexed, instanced primitives.
    struct DrawIndexedDesc
    {
        /// Number of indices read from the index buffer for each instance.
        uint32_t indexCount;

        /// Number of instances to draw.
        uint32_t instanceCount;

        /// The location of the first index read by the GPU from the index buffer.
        uint32_t indexStart;

        /// A value added to each index before reading a vertex from the vertex buffer.
        int32_t baseVertex;

        /// A value added to each index before reading per-instance data from a vertex buffer.
        uint32_t baseInstance;
    };

    /// Information about an initialized device.
    struct DeviceInfo
    {
        /// The name of the API used to create this device.
        /// Will be one of the `he::rhi::Api_*` constants.
        StringView api{};

        /// The preferred shader model, usually the highest supported value.
        ShaderModel preferredShaderModel{ ShaderModel::None };

        /// List of the supported shader models. True values indicate that model is supported.
        bool supportedShaderModels[static_cast<uint32_t>(ShaderModel::_Count)]{};

        /// Information about the adapter utilized by the device.
        AdapterInfo adapter{};

        /// Alignment requirements for a texture upload buffer.
        uint32_t uploadDataAlignment{ 512 };

        /// Alignment requirements for row pitch of a texture upload buffer.
        uint32_t uploadDataPitchAlignment{ 256 };

        /// Maximum size of a dimension of a one dimension texture.
        uint32_t max1dTextureSize{ 0 };

        /// Maximum size of a dimension of a two dimensional texture.
        uint32_t max2dTextureSize{ 0 };

        /// Maximum size of a dimension of a three dimensional texture.
        uint32_t max3dTextureSize{ 0 };

        /// Maximum size of a dimension of a cube texture.
        uint32_t maxCubeTextureSize{ 0 };

        /// Maximum number of array layers in a texture.
        uint32_t maxTextureLayers{ 0 };
    };
}
