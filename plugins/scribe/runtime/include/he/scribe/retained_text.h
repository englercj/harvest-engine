// Copyright Chad Engler

#pragma once

#include "he/scribe/context.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/renderer.h"

#include "he/core/span.h"

namespace he::scribe
{
    enum RetainedTextDrawFlags : uint32_t
    {
        RetainedTextDrawFlagUseForegroundColor = 0x01u,
        RetainedTextDrawFlagStroke = 0x02u,
    };

    struct RetainedTextDraw
    {
        uint32_t fontFaceIndex{ 0 };
        uint32_t glyphIndex{ 0 };
        uint32_t flags{ 0 };
        Vec2f position{ 0.0f, 0.0f };
        Vec2f size{ 1.0f, 1.0f };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec2f basisX{ 1.0f, 0.0f };
        Vec2f basisY{ 0.0f, 1.0f };
        Vec2f offset{ 0.0f, 0.0f };
        StrokeStyle strokeStyle{};
    };

    struct RetainedTextQuad
    {
        Vec2f position{ 0.0f, 0.0f };
        Vec2f size{ 1.0f, 1.0f };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec2f basisX{ 1.0f, 0.0f };
        Vec2f basisY{ 0.0f, 1.0f };
        Vec2f offset{ 0.0f, 0.0f };
    };

    struct RetainedTextBuildDesc
    {
        ScribeContext* context{ nullptr };
        Span<const FontFaceHandle> fontFaces{};
        const LayoutResult* layout{ nullptr };
        float fontSize{ 16.0f };
        bool darkBackgroundPreferred{ true };
        Span<const TextStyle> styles{};
    };

    struct RetainedTextCachedBatch
    {
        const GlyphAtlas* atlas{ nullptr };
        uint32_t vertexCount{ 0 };
    };

    class RetainedTextModel
    {
    public:
        bool Build(const RetainedTextBuildDesc& desc);
        void Clear();

        bool IsEmpty() const { return m_draws.IsEmpty() && m_quads.IsEmpty(); }
        uint32_t GetDrawCount() const { return m_draws.Size(); }
        uint32_t GetQuadCount() const { return m_quads.Size(); }
        uint32_t GetEstimatedVertexCount() const { return m_estimatedVertexCount; }
        Span<const RetainedTextDraw> GetDraws() const { return m_draws; }
        Span<const RetainedTextQuad> GetQuads() const { return m_quads; }
        ScribeContext* GetContext() const { return m_context; }
        Vec2f GetOrigin() const { return m_origin; }
        float GetScale() const { return m_scale; }
        Vec4f GetForegroundColor() const { return m_foregroundColor; }
        void SetOrigin(const Vec2f& origin);
        void SetScale(float scale);
        void SetForegroundColor(const Vec4f& color);
        FontFaceHandle GetFontFaceHandle(uint32_t fontFaceIndex) const;
        uint32_t GetCachedVertexCount() const { return m_cachedVertices.Size(); }
        uint32_t GetCachedBatchCount() const { return m_cachedBatches.Size(); }
        uint32_t GetCachedQuadVertexCount() const { return m_cachedQuadVertices.Size(); }
        uint32_t GetGeometryCacheGeneration() const { return m_geometryCacheGeneration; }

    private:
        friend class Renderer;

        bool UpdateRenderData() const;
        const GlyphResource* GetPreparedGlyphResource(uint32_t drawIndex) const;
        void ClearPreparedGlyphResources() const;
        void ClearTransformedVertexCache() const;
        void InvalidateGeometry() const;
        void InvalidateColor() const;

    private:
        ScribeContext* m_context{ nullptr };
        Vector<FontFaceHandle> m_fontFaces{};
        Vector<RetainedTextDraw> m_draws{};
        Vector<RetainedTextQuad> m_quads{};
        Vec2f m_origin{ 0.0f, 0.0f };
        float m_scale{ 1.0f };
        Vec4f m_foregroundColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        mutable Vector<GlyphResource> m_preparedGlyphs{};
        mutable Vector<PackedGlyphVertex> m_cachedVertices{};
        mutable Vector<RetainedTextCachedBatch> m_cachedBatches{};
        mutable Vector<PackedQuadVertex> m_cachedQuadVertices{};
        mutable bool m_hasCachedGeometry{ false };
        mutable bool m_hasCachedColor{ false };
        mutable uint32_t m_geometryCacheGeneration{ 0 };
        uint32_t m_estimatedVertexCount{ 0 };
    };
}
