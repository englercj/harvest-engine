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
    struct GlyphAtlas;
    class RetainedTextModel;
    class RetainedVectorImageModel;
    struct RetainedTextInstanceDesc;
    struct RetainedVectorImageInstanceDesc;

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
    };

    struct GlyphResource
    {
        PackedGlyphVertex vertices[ScribeGlyphVertexCount]{};
        GlyphAtlas* atlas{ nullptr };
        uint32_t vertexCount{ 0 };
    };

    struct FrameDesc
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
        rhi::TextureState targetState{ rhi::TextureState::Common };
        Vec2u targetSize{ 0, 0 };
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

        bool BeginFrame(const FrameDesc& desc);
        bool PrepareRetainedText(const RetainedTextModel& text);
        bool PrepareRetainedVectorImage(const RetainedVectorImageModel& image);
        void SetGlyphBatchingEnabled(bool enabled) { m_glyphBatchingEnabled = enabled; }
        bool IsGlyphBatchingEnabled() const { return m_glyphBatchingEnabled; }
        void ReserveQueuedVertexCapacity(uint32_t vertexCount, uint32_t batchCount = 0);
        void QueueDraw(const DrawGlyphDesc& desc);
        void QueueQuad(const DrawQuadDesc& desc);
        void QueueRetainedText(const RetainedTextModel& text, const RetainedTextInstanceDesc& instance);
        void QueueRetainedVectorImage(const RetainedVectorImageModel& image, const RetainedVectorImageInstanceDesc& instance);
        void EndFrame();
        uint32_t GetLastSubmittedDrawCount() const { return m_lastSubmittedDrawCount; }

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
        FrameDesc m_frame{};
        StreamBuffer m_streamBuffers[rhi::MaxFrameCount]{};
        StreamBuffer m_quadStreamBuffers[rhi::MaxFrameCount]{};
        uint32_t m_streamBufferIndex{ 0 };
        Vector<StreamBatch> m_batches{};
        Vector<PackedGlyphVertex> m_streamVertices{};
        Vector<PackedQuadVertex> m_quadVertices{};
        bool m_glyphBatchingEnabled{ true };
        uint32_t m_lastSubmittedDrawCount{ 0 };
    };
}
