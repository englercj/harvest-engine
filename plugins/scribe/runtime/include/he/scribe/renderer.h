// Copyright Chad Engler

#pragma once

#include "he/scribe/runtime_blob.h"

#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/math/types.h"
#include "he/rhi/types.h"

namespace he::scribe
{
    struct PackedGlyphVertex
    {
        Vec4f pos{ 0, 0, 0, 0 };
        Vec4f tex{ 0, 0, 0, 0 };
        Vec4f jac{ 0, 0, 0, 0 };
        Vec4f bnd{ 0, 0, 0, 0 };
        Vec4f col{ 0, 0, 0, 0 };
    };

    struct TextureDataDesc
    {
        const void* data{ nullptr };
        Vec2u size{ 0, 0 };
        uint32_t rowPitch{ 0 };
    };

    struct GlyphResourceCreateInfo
    {
        const PackedGlyphVertex* vertices{ nullptr };
        uint32_t vertexCount{ 0 };
        TextureDataDesc curveTexture{};
        TextureDataDesc bandTexture{};
    };

    struct GlyphResource
    {
        rhi::Buffer* vertexBuffer{ nullptr };
        rhi::Texture* curveTexture{ nullptr };
        rhi::TextureView* curveView{ nullptr };
        rhi::Texture* bandTexture{ nullptr };
        rhi::TextureView* bandView{ nullptr };
        rhi::DescriptorTable* descriptorTable{ nullptr };
        uint32_t vertexCount{ 0 };
    };

    struct FrameDesc
    {
        rhi::RenderCmdList* cmdList{ nullptr };
        const rhi::RenderTargetView* targetView{ nullptr };
        rhi::TextureState targetState{ rhi::TextureState::Common };
        Vec2u targetSize{ 0, 0 };
        bool clearTarget{ false };
        Vec4f clearColor{ 0, 0, 0, 0 };
    };

    struct DrawGlyphDesc
    {
        const GlyphResource* glyph{ nullptr };
        Vec2f position{ 0, 0 };
        Vec2f size{ 1, 1 };
        Vec4f color{ 1, 1, 1, 1 };
        Vec2f basisX{ 1, 0 };
        Vec2f basisY{ 0, 1 };
        Vec2f offset{ 0, 0 };
    };

    class Renderer
    {
    public:
        Renderer() = default;
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        ~Renderer() noexcept;

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        bool Initialize(rhi::Device& device, rhi::Format targetFormat);
        void Terminate();

        bool IsInitialized() const { return m_device != nullptr; }

        bool CreateGlyphResource(GlyphResource& out, const GlyphResourceCreateInfo& desc);
        bool CreateCompiledGlyphResource(
            GlyphResource& out,
            const LoadedFontFaceBlob& fontFace,
            uint32_t glyphIndex);
        bool CreateDebugGlyphResource(GlyphResource& out);
        void DestroyGlyphResource(GlyphResource& resource);

        bool BeginFrame(const FrameDesc& desc);
        void QueueDraw(const DrawGlyphDesc& desc);
        void EndFrame();

    private:
        struct QueuedDraw
        {
            const GlyphResource* glyph{ nullptr };
            Vec2f position{ 0, 0 };
            Vec2f size{ 1, 1 };
            Vec4f color{ 1, 1, 1, 1 };
            Vec2f basisX{ 1, 0 };
            Vec2f basisY{ 0, 1 };
            Vec2f offset{ 0, 0 };
        };

        bool CreateDeviceResources();
        void DestroyDeviceResources();
        void EmitDraw(const QueuedDraw& draw);

    private:
        rhi::Device* m_device{ nullptr };
        rhi::RootSignature* m_rootSignature{ nullptr };
        rhi::VertexBufferFormat* m_vertexBufferFormat{ nullptr };
        rhi::RenderPipeline* m_pipeline{ nullptr };
        rhi::Format m_targetFormat{ rhi::Format::Invalid };
        FrameDesc m_frame{};
        Vector<QueuedDraw> m_draws{};
    };
}
