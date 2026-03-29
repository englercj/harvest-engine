// Copyright Chad Engler

#pragma once

#include "he/scribe/context.h"
#include "he/scribe/retained_text.h"
#include "he/scribe/schema_types.h"

#include "he/core/vector.h"
#include "he/math/types.h"

#include <cstdint>

namespace he::scribe
{
    struct GlyphAtlas;

    enum RetainedVectorImageDrawFlags : uint32_t
    {
        RetainedVectorImageDrawFlagStroke = 0x01u,
        RetainedVectorImageDrawFlagRuntimeRestroke = 0x02u,
    };

    enum class RetainedVectorImageShapeResourceKind : uint8_t
    {
        CompiledShape,
        RuntimeRestroke,
    };

    struct RetainedVectorImageDraw
    {
        uint32_t shapeIndex{ 0 };
        uint32_t flags{ 0 };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec2f offset{ 0.0f, 0.0f };
        StrokeStyle strokeStyle{};
    };

    struct RetainedVectorImageBuildDesc
    {
        ScribeContext* context{ nullptr };
        VectorImageResourceReader image{};
        Span<const schema::Word> imageWords{};
        bool includeFill{ true };
        Vec4f strokeColor{ 0.0f, 0.0f, 0.0f, 0.0f };
        StrokeStyle strokeStyle{};
    };

    struct RetainedVectorImageCachedBatch
    {
        const GlyphAtlas* atlas{ nullptr };
        uint32_t vertexCount{ 0 };
    };

    class RetainedVectorImageModel
    {
    public:
        bool Build(const RetainedVectorImageBuildDesc& desc);
        void Clear();

        bool IsEmpty() const { return m_draws.IsEmpty() && m_textDraws.IsEmpty(); }
        uint32_t GetDrawCount() const { return m_draws.Size() + m_textDraws.Size(); }
        uint32_t GetShapeDrawCount() const { return m_draws.Size(); }
        uint32_t GetTextDrawCount() const { return m_textDraws.Size(); }
        uint32_t GetEstimatedVertexCount() const { return m_estimatedVertexCount; }
        Span<const RetainedVectorImageDraw> GetDraws() const { return m_draws; }
        Span<const RetainedTextDraw> GetTextDraws() const { return m_textDraws; }
        ScribeContext* GetContext() const { return m_context; }
        VectorImageResourceReader GetImage() const { return m_image; }
        Vec2f GetViewBoxSize() const { return m_viewBoxSize; }
        Vec2f GetOrigin() const { return m_origin; }
        float GetScale() const { return m_scale; }
        Vec4f GetTint() const { return m_tint; }
        void SetOrigin(const Vec2f& origin);
        void SetScale(float scale);
        void SetTint(const Vec4f& tint);
        FontFaceHandle GetFontFaceHandle(uint32_t fontFaceIndex) const;
        bool TryGetPreparedShapeResource(
            uint32_t shapeIndex,
            RetainedVectorImageShapeResourceKind resourceKind,
            const StrokeStyle& style,
            Renderer& renderer,
            const GlyphResource*& out) const;
        uint32_t GetCachedVertexCount() const { return m_cachedVertices.Size(); }
        uint32_t GetCachedBatchCount() const { return m_cachedBatches.Size(); }
        uint32_t GetGeometryCacheGeneration() const { return m_geometryCacheGeneration; }

    private:
        friend class Renderer;

        bool UpdateRenderData(Renderer& renderer) const;
        void ClearTransformedVertexCache() const;
        void InvalidateGeometry() const;
        void InvalidateColor() const;

    private:
        ScribeContext* m_context{ nullptr };
        Vector<schema::Word> m_imageWords{};
        VectorImageResourceReader m_image{};
        Vector<FontFaceHandle> m_fontFaces{};
        Vector<RetainedVectorImageDraw> m_draws{};
        Vector<RetainedTextDraw> m_textDraws{};
        Vec2f m_origin{ 0.0f, 0.0f };
        float m_scale{ 1.0f };
        Vec4f m_tint{ 1.0f, 1.0f, 1.0f, 1.0f };
        mutable Vector<GlyphResource> m_shapeResources{};
        mutable Vector<GlyphResource> m_runtimeStrokeResources{};
        mutable GlyphAtlas* m_sharedShapeAtlas{ nullptr };
        mutable Vector<PackedGlyphVertex> m_cachedVertices{};
        mutable Vector<RetainedVectorImageCachedBatch> m_cachedBatches{};
        mutable bool m_hasCachedGeometry{ false };
        mutable bool m_hasCachedColor{ false };
        mutable uint32_t m_geometryCacheGeneration{ 0 };
        Vec2f m_viewBoxSize{ 0.0f, 0.0f };
        uint32_t m_estimatedVertexCount{ 0 };
    };
}
