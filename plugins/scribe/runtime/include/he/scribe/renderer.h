// Copyright Chad Engler

#pragma once

#include "he/scribe/context.h"
#include "he/scribe/packed_data.h"
#include "he/scribe/schema_types.h"

#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/math/types.h"
#include "he/rhi/types.h"

namespace he::scribe
{
    // The draw-oriented API can submit multiple scribe passes before the caller advances to the
    // next frame fence, so keep a deeper upload-buffer ring than MaxFrameCount alone.
    constexpr uint32_t ScribeStreamBufferRingSize = rhi::MaxFrameCount * 8u;

    struct GlyphAtlas;
    class RetainedTextModel;
    class RetainedVectorImageModel;

    struct PackedGlyphVertex
    {
        Vec4f pos{ 0, 0, 0, 0 };
        Vec4f tex{ 0, 0, 0, 0 };
        Vec4f jac{ 0, 0, 0, 0 };
        Vec4f bnd{ 0, 0, 0, 0 };
        Vec4f col{ 0, 0, 0, 0 };
    };

    struct PackedQuadVertex
    {
        Vec2f pos{ 0, 0 };
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
        bool vertexColorIsWhite{ true };
    };

    struct GlyphResource
    {
        PackedGlyphVertex vertices[ScribeGlyphVertexCount]{};
        GlyphAtlas* atlas{ nullptr };
        uint32_t vertexCount{ 0 };
        bool vertexColorIsWhite{ true };
    };

    struct ViewTransform2D
    {
        Vec2f position{ 0.0f, 0.0f };
        Vec2f scale{ 1.0f, 1.0f };
        float rotationRadians{ 0.0f };
        float skewX{ 0.0f };
    };

    struct DrawPassDesc
    {
        struct GpuTimerDesc
        {
            rhi::TimestampQuerySet* querySet{ nullptr };
            rhi::Buffer* resolveBuffer{ nullptr };
            uint32_t startQueryIndex{ 0 };
            uint32_t endQueryIndex{ 1 };
            uint32_t resolveBufferOffset{ 0 };
        };

        rhi::RenderCmdList* cmdList{ nullptr };
        const rhi::RenderTargetView* targetView{ nullptr };
        // External texture state before BeginDraw and after EndDraw. The renderer transitions
        // into RenderTarget for the pass and restores this state when the pass ends.
        rhi::TextureState targetState{ rhi::TextureState::Common };
        Vec2u targetSize{ 0, 0 };
        ViewTransform2D viewTransform{};
        bool clearTarget{ false };
        Vec4f clearColor{ 0, 0, 0, 0 };
        GpuTimerDesc gpuTimer{};
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

    struct DrawQuadDesc
    {
        Vec2f position{ 0, 0 };
        Vec2f size{ 1, 1 };
        Vec4f color{ 1, 1, 1, 1 };
        Vec2f basisX{ 1, 0 };
        Vec2f basisY{ 0, 1 };
        Vec2f offset{ 0, 0 };
    };

    void BuildFrameConstants(
        float* outConstants,
        const Vec2u& targetSize,
        const ViewTransform2D& viewTransform);
    void TransformDrawVertices(PackedGlyphVertex* dst, const DrawGlyphDesc& draw);
    void UpdateDrawVertexColors(PackedGlyphVertex* dst, const DrawGlyphDesc& draw);
    void AppendQuadVertices(Vector<PackedQuadVertex>& out, const DrawQuadDesc& desc);
    void UpdateQuadVertexColors(PackedQuadVertex* dst, const Vec4f& color, uint32_t vertexCount);

    class Renderer
    {
    public:
        explicit Renderer(ScribeContext& context) noexcept
            : m_context(context)
        {}
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        ~Renderer() noexcept;

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        bool Initialize(rhi::Format targetFormat);
        void Terminate();

        bool IsInitialized() const { return m_device != nullptr; }

        bool CreateGlyphResource(GlyphResource& out, const GlyphResourceCreateInfo& desc);
        bool CreateDebugGlyphResource(GlyphResource& out);
        void DestroyGlyphResource(GlyphResource& resource);

        void SetGlyphBatchingEnabled(bool enabled) { m_glyphBatchingEnabled = enabled; }
        bool IsGlyphBatchingEnabled() const { return m_glyphBatchingEnabled; }
        uint32_t GetLastSubmittedDrawCount() const { return m_lastSubmittedDrawCount; }

        bool BeginDraw(const DrawPassDesc& desc);
        void DrawGlyph(const DrawGlyphDesc& desc);
        void DrawQuad(const DrawQuadDesc& desc);
        void DrawText(const RetainedTextModel& text);
        void DrawImage(const RetainedVectorImageModel& image);
        void EndDraw();

    private:
        struct StreamBatch
        {
            const GlyphAtlas* atlas{ nullptr };
            uint32_t vertexStart{ 0 };
            uint32_t vertexCount{ 0 };
        };

        struct StreamBuffer
        {
            rhi::Buffer* buffer{ nullptr };
            uint32_t size{ 0 };
        };

        bool CreateDedicatedAtlas(
            GlyphAtlas*& out,
            const TextureDataDesc& curveTexture,
            const TextureDataDesc& bandTexture);
        void ReleaseAtlas(GlyphAtlas*& atlas);
        void EnsureQueuedCapacity(uint32_t vertexCount, uint32_t batchCount = 0);
        bool EnsureStreamBufferCapacity(StreamBuffer& streamBuffer, uint32_t minSize, uint32_t stride, const char* name);
        bool CreateDeviceResources();
        void DestroyDeviceResources();
        void AppendDrawVertices(const DrawGlyphDesc& draw);

    private:
        ScribeContext& m_context;
        rhi::Device* m_device{ nullptr };
        rhi::RootSignature* m_rootSignature{ nullptr };
        rhi::VertexBufferFormat* m_vertexBufferFormat{ nullptr };
        rhi::RenderPipeline* m_pipeline{ nullptr };
        rhi::RootSignature* m_quadRootSignature{ nullptr };
        rhi::VertexBufferFormat* m_quadVertexBufferFormat{ nullptr };
        rhi::RenderPipeline* m_quadPipeline{ nullptr };
        rhi::Format m_targetFormat{ rhi::Format::Invalid };
        DrawPassDesc m_drawPass{};
        StreamBuffer m_streamBuffers[ScribeStreamBufferRingSize]{};
        StreamBuffer m_quadStreamBuffers[ScribeStreamBufferRingSize]{};
        uint32_t m_streamBufferIndex{ 0 };
        Vector<StreamBatch> m_batches{};
        Vector<PackedGlyphVertex> m_streamVertices{};
        Vector<PackedQuadVertex> m_quadVertices{};
        bool m_glyphBatchingEnabled{ true };
        uint32_t m_lastSubmittedDrawCount{ 0 };
    };
}
