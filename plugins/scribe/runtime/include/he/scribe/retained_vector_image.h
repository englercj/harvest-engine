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
    class RetainedVectorImageModel;

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

    class VectorImageBuilder
    {
    public:
        explicit VectorImageBuilder(float flatteningTolerance = 0.25f) noexcept
            : m_flatteningTolerance(flatteningTolerance)
        {
        }

        void Clear();

        void SetViewBox(float minX, float minY, float width, float height);
        void ClearViewBox();

        void SetFillStyle(const Vec4f& color) { m_fillStyle = color; }
        void SetStrokeStyle(const Vec4f& color) { m_strokeColor = color; }
        void SetLineWidth(float width) { m_strokeStyle.width = width; }
        void SetLineJoin(StrokeJoinStyle joinStyle) { m_strokeStyle.joinStyle = joinStyle; }
        void SetLineCap(StrokeCapStyle capStyle) { m_strokeStyle.capStyle = capStyle; }
        void SetMiterLimit(float miterLimit) { m_strokeStyle.miterLimit = miterLimit; }
        void SetFillRule(FillRule fillRule) { m_fillRule = fillRule; }
        void SetFlatteningTolerance(float flatteningTolerance) { m_flatteningTolerance = flatteningTolerance; }

        void BeginPath();
        void MoveTo(float x, float y);
        void MoveTo(const Vec2f& point) { MoveTo(point.x, point.y); }
        void LineTo(float x, float y);
        void LineTo(const Vec2f& point) { LineTo(point.x, point.y); }
        void QuadraticCurveTo(float cx, float cy, float x, float y);
        void QuadraticCurveTo(const Vec2f& control, const Vec2f& point) { QuadraticCurveTo(control.x, control.y, point.x, point.y); }
        void BezierCurveTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
        void BezierCurveTo(const Vec2f& control1, const Vec2f& control2, const Vec2f& point)
        {
            BezierCurveTo(control1.x, control1.y, control2.x, control2.y, point.x, point.y);
        }
        void ClosePath();

        void Rect(float x, float y, float width, float height);
        void Rect(const Vec2f& position, const Vec2f& size) { Rect(position.x, position.y, size.x, size.y); }
        void Ellipse(float cx, float cy, float radiusX, float radiusY);
        void Ellipse(const Vec2f& center, float radiusX, float radiusY) { Ellipse(center.x, center.y, radiusX, radiusY); }
        void Line(float x0, float y0, float x1, float y1);
        void Line(const Vec2f& from, const Vec2f& to) { Line(from.x, from.y, to.x, to.y); }
        void Polyline(Span<const Vec2f> points);
        void Polygon(Span<const Vec2f> points);

        bool Fill();
        bool Stroke();

        bool Build(Vector<schema::Word>& outWords, VectorImageResourceReader& outImage) const;
        bool Build(RetainedVectorImageModel& outImage, ScribeContext& context) const;

    private:
        enum class PathCommandType : uint8_t
        {
            MoveTo,
            LineTo,
            QuadraticCurveTo,
            BezierCurveTo,
            ClosePath,
        };

        struct PathCommand
        {
            PathCommandType type{ PathCommandType::MoveTo };
            Vec2f p0{ 0.0f, 0.0f };
            Vec2f p1{ 0.0f, 0.0f };
            Vec2f p2{ 0.0f, 0.0f };
        };

        struct DrawPath
        {
            Vector<PathCommand> commands{};
            bool fill{ false };
            bool stroke{ false };
            FillRule fillRule{ FillRule::NonZero };
            Vec4f fillStyle{ 0.0f, 0.0f, 0.0f, 1.0f };
            Vec4f strokeColor{ 0.0f, 0.0f, 0.0f, 1.0f };
            StrokeStyle strokeStyle{ 1.0f, StrokeJoinStyle::Miter, StrokeCapStyle::Butt, 4.0f };
        };

    private:
        Vector<DrawPath> m_drawPaths{};
        Vector<PathCommand> m_currentPath{};
        Vec4f m_fillStyle{ 0.0f, 0.0f, 0.0f, 1.0f };
        Vec4f m_strokeColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        StrokeStyle m_strokeStyle{ 1.0f, StrokeJoinStyle::Miter, StrokeCapStyle::Butt, 4.0f };
        FillRule m_fillRule{ FillRule::NonZero };
        float m_viewBoxMinX{ 0.0f };
        float m_viewBoxMinY{ 0.0f };
        float m_viewBoxWidth{ 0.0f };
        float m_viewBoxHeight{ 0.0f };
        float m_flatteningTolerance{ 0.25f };
        bool m_hasViewBox{ false };
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
        RetainedAabb GetAabb() const { return m_aabb; }
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
        void RebuildLocalAabb(Renderer& renderer);
        void UpdateAabbFromLocal();
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
        RetainedAabb m_localAabb{};
        RetainedAabb m_aabb{};
        uint32_t m_estimatedVertexCount{ 0 };
    };
}
