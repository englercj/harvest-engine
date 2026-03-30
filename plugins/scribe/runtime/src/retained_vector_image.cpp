// Copyright Chad Engler

#include "he/scribe/retained_vector_image.h"

#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/packed_data.h"
#include "he/scribe/retained_text.h"
#include "he/scribe/renderer.h"

#include "glyph_atlas_utils.h"

#include "../../editor/src/image_compile_geometry.h"
#include "../../editor/src/outline_build_utils.h"
#include "../../editor/src/packed_geometry_compile_utils.h"
#include "../../editor/src/resource_build_utils.h"

#include "he/core/log.h"
#include "he/core/math.h"
#include "he/core/memory_ops.h"
#include "he/schema/schema.h"
#include "he/scribe/stroke_outline.h"

namespace he::scribe
{
    namespace
    {
        constexpr float RuntimeVectorImageBandOverlapEpsilon = 1.0f / 1024.0f;
        constexpr float RuntimeEllipseKappa = 0.5522847498307936f;

        bool EqualColor(const Vec4f& a, const Vec4f& b)
        {
            return (a.x == b.x)
                && (a.y == b.y)
                && (a.z == b.z)
                && (a.w == b.w);
        }

        StrokeJoinStyle ToRuntimeStrokeJoin(StrokeJoinKind value)
        {
            switch (value)
            {
                case StrokeJoinKind::Bevel: return StrokeJoinStyle::Bevel;
                case StrokeJoinKind::Round: return StrokeJoinStyle::Round;
                case StrokeJoinKind::Miter:
                default:
                    return StrokeJoinStyle::Miter;
            }
        }

        StrokeCapStyle ToRuntimeStrokeCap(StrokeCapKind value)
        {
            switch (value)
            {
                case StrokeCapKind::Square: return StrokeCapStyle::Square;
                case StrokeCapKind::Round: return StrokeCapStyle::Round;
                case StrokeCapKind::Butt:
                default:
                    return StrokeCapStyle::Butt;
            }
        }

        Vec4f MultiplyColor(const Vec4f& a, const Vec4f& b)
        {
            return {
                a.x * b.x,
                a.y * b.y,
                a.z * b.z,
                a.w * b.w
            };
        }

        void ExpandAabb(RetainedAabb& aabb, const Vec2f& point)
        {
            if (aabb.IsEmpty())
            {
                aabb.min = point;
                aabb.max = point;
                return;
            }

            aabb.min.x = Min(aabb.min.x, point.x);
            aabb.min.y = Min(aabb.min.y, point.y);
            aabb.max.x = Max(aabb.max.x, point.x);
            aabb.max.y = Max(aabb.max.y, point.y);
        }

        StrokeJoinKind ToSchemaStrokeJoin(StrokeJoinStyle value)
        {
            switch (value)
            {
                case StrokeJoinStyle::Bevel: return StrokeJoinKind::Bevel;
                case StrokeJoinStyle::Round: return StrokeJoinKind::Round;
                case StrokeJoinStyle::Miter:
                default:
                    return StrokeJoinKind::Miter;
            }
        }

        StrokeCapKind ToSchemaStrokeCap(StrokeCapStyle value)
        {
            switch (value)
            {
                case StrokeCapStyle::Square: return StrokeCapKind::Square;
                case StrokeCapStyle::Round: return StrokeCapKind::Round;
                case StrokeCapStyle::Butt:
                default:
                    return StrokeCapKind::Butt;
            }
        }

        bool BuildStrokedFillCurves(
            Vector<editor::curve_compile::CurveData>& out,
            Span<const editor::StrokeSourcePoint> strokePoints,
            Span<const editor::StrokeSourceCommand> strokeCommands,
            const StrokeStyle& style,
            float flatteningTolerance)
        {
            out.Clear();
            if (!style.IsVisible() || strokePoints.IsEmpty() || strokeCommands.IsEmpty())
            {
                return false;
            }

            Vector<StrokeOutlineSourcePoint> outlinePoints{};
            outlinePoints.Resize(strokePoints.Size());
            for (uint32_t pointIndex = 0; pointIndex < strokePoints.Size(); ++pointIndex)
            {
                outlinePoints[pointIndex].x = strokePoints[pointIndex].x;
                outlinePoints[pointIndex].y = strokePoints[pointIndex].y;
            }

            Vector<StrokeOutlineSourceCommand> outlineCommands{};
            outlineCommands.Resize(strokeCommands.Size());
            for (uint32_t commandIndex = 0; commandIndex < strokeCommands.Size(); ++commandIndex)
            {
                outlineCommands[commandIndex].type = strokeCommands[commandIndex].type;
                outlineCommands[commandIndex].firstPoint = strokeCommands[commandIndex].firstPoint;
            }

            StrokeOutlineStyle outlineStyle{};
            outlineStyle.width = style.width;
            outlineStyle.join = ToSchemaStrokeJoin(style.joinStyle);
            outlineStyle.cap = ToSchemaStrokeCap(style.capStyle);
            outlineStyle.miterLimit = style.miterLimit;

            Vector<StrokeOutlineCurve> strokedCurves{};
            if (!BuildStrokedOutlineCurves(
                    strokedCurves,
                    Span<const StrokeOutlineSourcePoint>(outlinePoints.Data(), outlinePoints.Size()),
                    Span<const StrokeOutlineSourceCommand>(outlineCommands.Data(), outlineCommands.Size()),
                    outlineStyle))
            {
                return false;
            }

            editor::curve_compile::CurveBuilder builder(flatteningTolerance);
            for (const StrokeOutlineCurve& curve : strokedCurves)
            {
                const editor::curve_compile::Point2 p0{ curve.x0, curve.y0 };
                switch (curve.kind)
                {
                    case StrokeOutlineCurveKind::Line:
                        builder.AddLine(p0, { curve.x1, curve.y1 });
                        break;

                    case StrokeOutlineCurveKind::Quadratic:
                        builder.AddQuadratic(p0, { curve.x1, curve.y1 }, { curve.x2, curve.y2 });
                        break;

                    case StrokeOutlineCurveKind::Cubic:
                    default:
                        builder.AddCubic(p0, { curve.x1, curve.y1 }, { curve.x2, curve.y2 }, { curve.x3, curve.y3 });
                        break;
                }
            }

            out = Move(builder.Curves());
            return !out.IsEmpty();
        }

        bool AppendCompiledVectorShape(
            editor::CompiledVectorImageData& outImageData,
            Span<const editor::curve_compile::CurveData> curves,
            Span<const editor::StrokeSourcePoint> strokePoints,
            Span<const editor::StrokeSourceCommand> strokeCommands,
            FillRule fillRule,
            uint32_t& outShapeIndex)
        {
            editor::CompiledVectorShapeRenderEntry& compiledShape = outImageData.shapes.EmplaceBack();
            editor::PackedBandStats bandStats{};
            editor::CompiledShapeBuildOptions options{};
            options.bandOverlapEpsilon = outImageData.bandOverlapEpsilon;
            options.normalizeOriginToBoundsMin = true;
            options.useSingleBandForSmallNonZeroShape = true;
            if (!editor::BuildCompiledShapeGeometry(
                    compiledShape,
                    outImageData.curveTexels,
                    outImageData.bandTexels,
                    outImageData.strokePoints,
                    outImageData.strokeCommands,
                    bandStats,
                    curves,
                    strokePoints,
                    strokeCommands,
                    fillRule,
                    options))
            {
                outImageData.shapes.Resize(outImageData.shapes.Size() - 1u);
                return false;
            }

            outImageData.bandHeaderCount += bandStats.headerCount;
            outImageData.emittedBandPayloadTexelCount += bandStats.emittedPayloadTexelCount;
            outImageData.reusedBandCount += bandStats.reusedBandCount;
            outImageData.reusedBandPayloadTexelCount += bandStats.reusedPayloadTexelCount;
            const float minX = compiledShape.originX + compiledShape.boundsMinX;
            const float minY = compiledShape.originY + compiledShape.boundsMinY;
            const float maxX = compiledShape.originX + compiledShape.boundsMaxX;
            const float maxY = compiledShape.originY + compiledShape.boundsMaxY;
            if (outImageData.shapes.Size() == 1u)
            {
                outImageData.boundsMinX = minX;
                outImageData.boundsMinY = minY;
                outImageData.boundsMaxX = maxX;
                outImageData.boundsMaxY = maxY;
            }
            else
            {
                outImageData.boundsMinX = Min(outImageData.boundsMinX, minX);
                outImageData.boundsMinY = Min(outImageData.boundsMinY, minY);
                outImageData.boundsMaxX = Max(outImageData.boundsMaxX, maxX);
                outImageData.boundsMaxY = Max(outImageData.boundsMaxY, maxY);
            }
            outShapeIndex = outImageData.shapes.Size() - 1u;
            return true;
        }

        DrawGlyphDesc BuildShapeDrawDesc(const RetainedVectorImageModel& model, const RetainedVectorImageDraw& draw, const GlyphResource& glyph)
        {
            DrawGlyphDesc desc{};
            desc.glyph = &glyph;
            desc.pixelShader = model.PixelShader();
            desc.position = model.GetOrigin();
            desc.size = { model.GetScale(), model.GetScale() };
            desc.color = MultiplyColor(draw.color, model.GetTint());
            desc.offset = draw.offset;
            return desc;
        }

        DrawGlyphDesc BuildLocalShapeDrawDesc(const RetainedVectorImageDraw& draw, const GlyphResource& glyph)
        {
            DrawGlyphDesc desc{};
            desc.glyph = &glyph;
            desc.position = { 0.0f, 0.0f };
            desc.size = { 1.0f, 1.0f };
            desc.color = draw.color;
            desc.offset = draw.offset;
            return desc;
        }

        RetainedVectorImageShapeResourceKind GetShapeResourceKind(const RetainedVectorImageDraw& draw)
        {
            return ((draw.flags & RetainedVectorImageDrawFlagRuntimeRestroke) != 0)
                ? RetainedVectorImageShapeResourceKind::RuntimeRestroke
                : RetainedVectorImageShapeResourceKind::CompiledShape;
        }

        DrawGlyphDesc BuildTextDrawDesc(const RetainedVectorImageModel& model, const RetainedTextDraw& draw, const GlyphResource& glyph)
        {
            DrawGlyphDesc desc{};
            desc.glyph = &glyph;
            desc.pixelShader = model.PixelShader();
            desc.position = {
                model.GetOrigin().x + (draw.position.x * model.GetScale()),
                model.GetOrigin().y + (draw.position.y * model.GetScale())
            };
            desc.size = {
                draw.size.x * model.GetScale(),
                draw.size.y * model.GetScale()
            };
            desc.color = MultiplyColor(draw.color, model.GetTint());
            desc.basisX = draw.basisX;
            desc.basisY = draw.basisY;
            desc.offset = draw.offset;
            return desc;
        }

        DrawGlyphDesc BuildLocalTextDrawDesc(const RetainedTextDraw& draw, const GlyphResource& glyph)
        {
            DrawGlyphDesc desc{};
            desc.glyph = &glyph;
            desc.position = draw.position;
            desc.size = draw.size;
            desc.color = draw.color;
            desc.basisX = draw.basisX;
            desc.basisY = draw.basisY;
            desc.offset = draw.offset;
            return desc;
        }

        Vec2f TransformTextRunVector(const ScribeImage::TextRun::Reader& run, const Vec2f& value)
        {
            return {
                (run.GetTransformXX() * value.x) + (run.GetTransformYX() * value.y),
                (run.GetTransformXY() * value.x) + (run.GetTransformYY() * value.y)
            };
        }

        Vec2f TransformTextRunPoint(const ScribeImage::TextRun::Reader& run, const Vec2f& value)
        {
            const Vec2f transformed = TransformTextRunVector(run, value);
            return {
                transformed.x + run.GetTransformTx(),
                transformed.y + run.GetTransformTy()
            };
        }

        float GetTextAnchorOffsetX(ScribeImage::TextAnchorKind anchor, float width)
        {
            switch (anchor)
            {
                case ScribeImage::TextAnchorKind::Middle: return -width * 0.5f;
                case ScribeImage::TextAnchorKind::End: return -width;
                case ScribeImage::TextAnchorKind::Start:
                default:
                    return 0.0f;
            }
        }

        void AppendTransformedTextRunDraws(
            Vector<RetainedTextDraw>& out,
            uint32_t& estimatedVertexCount,
            const ScribeContext& context,
            const RetainedTextModel& model,
            Span<const FontFaceHandle> layoutFontFaces,
            Span<const uint32_t> layoutFontFaceIndices,
            const ScribeImage::TextRun::Reader& run,
            const Vec2f& drawOffset,
            float anchorOffsetX,
            float baselineOffsetY)
        {
            for (const RetainedTextDraw& sourceDraw : model.GetDraws())
            {
                RetainedTextDraw& draw = out.EmplaceBack(sourceDraw);
                const uint32_t mappedFontFaceIndex =
                    (sourceDraw.fontFaceIndex < layoutFontFaceIndices.Size())
                    ? layoutFontFaceIndices[sourceDraw.fontFaceIndex]
                    : run.GetFontFaceIndex();
                draw.fontFaceIndex = mappedFontFaceIndex;

                float glyphOriginOffsetX = 0.0f;
                if (run.GetPositionUsesGlyphOriginX() && (sourceDraw.fontFaceIndex < layoutFontFaces.Size()))
                {
                    const ScribeFontFace::RuntimeResource::Reader fontFace = context.GetFontFace(layoutFontFaces[sourceDraw.fontFaceIndex]);
                    const FontFaceFillData::Reader fill = fontFace.IsValid() ? fontFace.GetFill() : FontFaceFillData::Reader{};
                    const schema::List<FontFaceGlyphRenderData>::Reader glyphs = fill.IsValid() ? fill.GetGlyphs() : schema::List<FontFaceGlyphRenderData>::Reader{};
                    if (glyphs.IsValid() && (sourceDraw.glyphIndex < glyphs.Size()))
                    {
                        glyphOriginOffsetX = glyphs[sourceDraw.glyphIndex].GetBoundsMinX() * sourceDraw.size.x;
                    }
                }

                const Vec2f localPosition{
                    run.GetPositionX() + anchorOffsetX + sourceDraw.position.x + (sourceDraw.size.x * sourceDraw.offset.x) - glyphOriginOffsetX,
                    (run.GetPositionY() - baselineOffsetY) + sourceDraw.position.y + (sourceDraw.size.y * sourceDraw.offset.y)
                };
                const Vec2f transformedPosition = TransformTextRunPoint(run, localPosition);
                draw.position = {
                    drawOffset.x + transformedPosition.x,
                    drawOffset.y + transformedPosition.y
                };
                draw.basisX = TransformTextRunVector(run, sourceDraw.basisX);
                draw.basisY = TransformTextRunVector(run, sourceDraw.basisY);
                draw.offset = { 0.0f, 0.0f };
                estimatedVertexCount += ScribeGlyphVertexCount;
            }
        }
    }

    bool RetainedVectorImageModel::Build(const RetainedVectorImageBuildDesc& desc)
    {
        Clear();

        if ((desc.context == nullptr) || !desc.image.IsValid())
        {
            return false;
        }

        if (!desc.imageWords.IsEmpty())
        {
            m_imageWords = desc.imageWords;
            m_image = schema::ReadRoot<VectorImageResource>(m_imageWords.Data());
        }
        else
        {
            m_image = desc.image;
        }

        const VectorImageResourceReader image = m_image;
        if (!image.IsValid() || !image.GetFill().IsValid() || !image.GetPaint().IsValid())
        {
            return false;
        }

        m_context = desc.context;

        const VectorImageRuntimeMetadata::Reader metadata = image.GetMetadata();
        m_viewBoxSize = {
            metadata.GetSourceViewBoxWidth(),
            metadata.GetSourceViewBoxHeight()
        };

        const VectorImagePaintData::Reader paint = image.GetPaint();
        const VectorImageFillData::Reader fill = image.GetFill();
        const ScribeImage::RuntimeResource::Text::Reader text = image.GetText();
        const schema::List<VectorImageLayer>::Reader layers = paint.GetLayers();
        const schema::List<schema::String>::Reader fontFaces = text.GetFontFaces();
        const schema::List<ScribeImage::TextRun>::Reader textRuns = text.GetRuns();
        const schema::List<VectorImageShapeRenderData>::Reader shapes = fill.GetShapes();
        m_shapeResources.Resize(shapes.Size(), DefaultInit);
        m_runtimeStrokeResources.Resize(shapes.Size(), DefaultInit);
        if (layers.IsEmpty() && textRuns.IsEmpty())
        {
            return false;
        }

        m_fontFaces.Resize(fontFaces.Size());
        for (uint32_t fontIndex = 0; fontIndex < fontFaces.Size(); ++fontIndex)
        {
            FontFaceHandle handle{};
            if (!desc.context->TryFindFontFaceByAlias(fontFaces[fontIndex].AsView(), handle))
            {
                HE_LOG_WARN(he_scribe,
                    HE_MSG("Skipping SVG text font alias that is not registered."),
                    HE_KV(alias, String(fontFaces[fontIndex].AsView())));
                continue;
            }

            if (!handle.IsValid())
            {
                HE_LOG_WARN(he_scribe,
                    HE_MSG("Skipping SVG text font alias that resolved to an invalid handle."),
                    HE_KV(alias, String(fontFaces[fontIndex].AsView())));
                continue;
            }

            m_fontFaces[fontIndex] = handle;
        }

        const Vec2f drawOffset{
            -metadata.GetSourceViewBoxMinX(),
            -metadata.GetSourceViewBoxMinY()
        };

        const bool addStroke = desc.strokeStyle.IsVisible() && (desc.strokeColor.w > 0.0f);
        m_draws.Reserve((desc.includeFill ? layers.Size() : 0u) + (addStroke ? layers.Size() : 0u));
        for (uint32_t layerIndex = 0; layerIndex < layers.Size(); ++layerIndex)
        {
            const VectorImageLayer::Reader layer = layers[layerIndex];
            if (layer.GetShapeIndex() >= shapes.Size())
            {
                return false;
            }

            const VectorImageShapeRenderData::Reader shape = shapes[layer.GetShapeIndex()];
            const Vec2f shapeOffset{
                drawOffset.x + shape.GetOriginX(),
                drawOffset.y + shape.GetOriginY()
            };
            const bool isStrokeLayer = layer.GetKind() == VectorLayerKind::Stroke;
            if (isStrokeLayer)
            {
                RetainedVectorImageDraw& stroke = m_draws.EmplaceBack();
                stroke.shapeIndex = layer.GetShapeIndex();
                // Authored SVG stroke layers are compiled to immutable shapes at import time.
                // They render through the same compiled-shape path as fills rather than going
                // back through the runtime restroker.
                stroke.flags = RetainedVectorImageDrawFlagStroke;
                stroke.color = {
                    layer.GetRed(),
                    layer.GetGreen(),
                    layer.GetBlue(),
                    layer.GetAlpha()
                };
                stroke.offset = shapeOffset;
                stroke.strokeStyle.width = layer.GetStrokeWidth();
                stroke.strokeStyle.joinStyle = ToRuntimeStrokeJoin(layer.GetStrokeJoin());
                stroke.strokeStyle.capStyle = ToRuntimeStrokeCap(layer.GetStrokeCap());
                stroke.strokeStyle.miterLimit = layer.GetStrokeMiterLimit();
                m_estimatedVertexCount += ScribeGlyphVertexCount;
                continue;
            }

            if (addStroke)
            {
                RetainedVectorImageDraw& stroke = m_draws.EmplaceBack();
                stroke.shapeIndex = layer.GetShapeIndex();
                stroke.flags = RetainedVectorImageDrawFlagStroke | RetainedVectorImageDrawFlagRuntimeRestroke;
                stroke.color = desc.strokeColor;
                stroke.offset = shapeOffset;
                stroke.strokeStyle = desc.strokeStyle;
                m_estimatedVertexCount += ScribeGlyphVertexCount;
            }

            if (desc.includeFill)
            {
                RetainedVectorImageDraw& draw = m_draws.EmplaceBack();
                draw.shapeIndex = layer.GetShapeIndex();
                draw.color = {
                    layer.GetRed(),
                    layer.GetGreen(),
                    layer.GetBlue(),
                    layer.GetAlpha()
                };
                draw.offset = shapeOffset;
                m_estimatedVertexCount += ScribeGlyphVertexCount;
            }
        }

        for (uint32_t runIndex = 0; runIndex < textRuns.Size(); ++runIndex)
        {
            const ScribeImage::TextRun::Reader srcRun = textRuns[runIndex];
            if (!srcRun.IsValid() || (srcRun.GetFontFaceIndex() >= m_fontFaces.Size()))
            {
                HE_LOG_WARN(he_scribe,
                    HE_MSG("Skipping SVG text run with invalid font-face index."),
                    HE_KV(font_face_index, srcRun.IsValid() ? srcRun.GetFontFaceIndex() : 0u),
                    HE_KV(font_face_count, m_fontFaces.Size()));
                continue;
            }

            const StringView runText = srcRun.GetText().AsView();
            if (runText.IsEmpty())
            {
                continue;
            }

            const float fontSize = Max(srcRun.GetFontSize(), 0.01f);
            const FontFaceHandle runFontFace = m_fontFaces[srcRun.GetFontFaceIndex()];
            if (!runFontFace.IsValid())
            {
                HE_LOG_WARN(he_scribe,
                    HE_MSG("Skipping SVG text run because its font alias was not resolved."),
                    HE_KV(font_face_index, srcRun.GetFontFaceIndex()),
                    HE_KV(text, String(runText)));
                continue;
            }

            Vector<FontFaceHandle> layoutFontFaces{};
            Vector<uint32_t> layoutFontFaceIndices{};
            uint32_t primaryLayoutFontFaceIndex = 0;
            for (uint32_t fontIndex = 0; fontIndex < m_fontFaces.Size(); ++fontIndex)
            {
                const FontFaceHandle handle = m_fontFaces[fontIndex];
                if (!handle.IsValid())
                {
                    continue;
                }

                uint32_t existingIndex = Limits<uint32_t>::Max;
                for (uint32_t layoutIndex = 0; layoutIndex < layoutFontFaces.Size(); ++layoutIndex)
                {
                    if (layoutFontFaces[layoutIndex] == handle)
                    {
                        existingIndex = layoutIndex;
                        break;
                    }
                }

                if (existingIndex == Limits<uint32_t>::Max)
                {
                    existingIndex = layoutFontFaces.Size();
                    layoutFontFaces.PushBack(handle);
                    layoutFontFaceIndices.PushBack(fontIndex);
                }

                if (fontIndex == srcRun.GetFontFaceIndex())
                {
                    primaryLayoutFontFaceIndex = existingIndex;
                }
            }

            TextStyle style{};
            style.fontFaceIndex = primaryLayoutFontFaceIndex;
            style.color = {
                srcRun.GetRed(),
                srcRun.GetGreen(),
                srcRun.GetBlue(),
                srcRun.GetAlpha()
            };
            if ((srcRun.GetStrokeWidth() > 0.0f) && (srcRun.GetStrokeAlpha() > 0.0f))
            {
                style.effects |= TextEffectFlags::Outline;
                style.outlineColor = {
                    srcRun.GetStrokeRed(),
                    srcRun.GetStrokeGreen(),
                    srcRun.GetStrokeBlue(),
                    srcRun.GetStrokeAlpha()
                };
                style.outlineWidthEm = srcRun.GetStrokeWidth() / (fontSize * 2.0f);
                style.outlineJoinStyle = ToRuntimeStrokeJoin(srcRun.GetStrokeJoin());
                style.outlineCapStyle = ToRuntimeStrokeCap(srcRun.GetStrokeCap());
                style.outlineMiterLimit = srcRun.GetStrokeMiterLimit();
            }

            TextStyleSpan styleSpan{};
            styleSpan.textByteEnd = runText.Size();
            styleSpan.styleIndex = 0;

            StyledTextLayoutDesc layoutDesc{};
            layoutDesc.fontFaces = layoutFontFaces;
            layoutDesc.text = runText;
            layoutDesc.options.fontSize = fontSize;
            layoutDesc.options.wrap = false;
            layoutDesc.styles = Span<const TextStyle>(&style, 1);
            layoutDesc.styleSpans = Span<const TextStyleSpan>(&styleSpan, 1);

            LayoutResult layout{};
            if (!desc.context->GetLayoutEngine().LayoutStyledText(layout, layoutDesc))
            {
                return false;
            }

            RetainedTextBuildDesc textBuildDesc{};
            textBuildDesc.context = desc.context;
            textBuildDesc.fontFaces = layoutFontFaces;
            textBuildDesc.layout = &layout;
            textBuildDesc.fontSize = fontSize;
            textBuildDesc.styles = Span<const TextStyle>(&style, 1);

            RetainedTextModel model{};
            if (!model.Build(textBuildDesc))
            {
                return false;
            }

            const float anchorOffsetX = GetTextAnchorOffsetX(srcRun.GetAnchor(), layout.width);
            const float baselineOffsetY = !layout.lines.IsEmpty() ? layout.lines[0].baselineY : 0.0f;
            AppendTransformedTextRunDraws(
                m_textDraws,
                m_estimatedVertexCount,
                *desc.context,
                model,
                layoutFontFaces,
                layoutFontFaceIndices,
                srcRun,
                drawOffset,
                anchorOffsetX,
                baselineOffsetY);
        }

        if (!m_draws.IsEmpty() || !m_textDraws.IsEmpty())
        {
            if (desc.context->IsInitialized() && desc.context->GetRenderer().IsInitialized())
            {
                RebuildLocalAabb(desc.context->GetRenderer());
                UpdateAabbFromLocal();
            }
            return true;
        }

        return false;
    }

    void RetainedVectorImageModel::Clear()
    {
        if (m_context)
        {
            GlyphAtlas* const sharedShapeAtlas = m_sharedShapeAtlas;
            bool releasedSharedShapeAtlas = false;
            if (m_context->IsInitialized() && m_context->GetRenderer().IsInitialized())
            {
                Renderer& renderer = m_context->GetRenderer();
                for (GlyphResource& resource : m_shapeResources)
                {
                    if ((resource.atlas != nullptr) && (resource.atlas == sharedShapeAtlas))
                    {
                        if (!releasedSharedShapeAtlas)
                        {
                            renderer.DestroyGlyphResource(resource);
                            releasedSharedShapeAtlas = true;
                        }
                        else
                        {
                            resource = {};
                        }

                        continue;
                    }

                    renderer.DestroyGlyphResource(resource);
                }

                for (GlyphResource& resource : m_runtimeStrokeResources)
                {
                    renderer.DestroyGlyphResource(resource);
                }
            }
            else if (rhi::Device* device = m_context->GetDevice())
            {
                for (GlyphResource& resource : m_shapeResources)
                {
                    if ((resource.atlas != nullptr) && (resource.atlas == sharedShapeAtlas))
                    {
                        if (!releasedSharedShapeAtlas)
                        {
                            DestroyAtlas(*device, resource.atlas);
                            releasedSharedShapeAtlas = true;
                        }
                        else
                        {
                            resource.atlas = nullptr;
                        }

                        resource = {};
                        continue;
                    }

                    DestroyAtlas(*device, resource.atlas);
                    resource = {};
                }

                for (GlyphResource& resource : m_runtimeStrokeResources)
                {
                    DestroyAtlas(*device, resource.atlas);
                    resource = {};
                }
            }
        }

        m_context = nullptr;
        m_imageWords.Clear();
        m_image = {};
        m_fontFaces.Clear();
        m_draws.Clear();
        m_textDraws.Clear();
        m_origin = {};
        m_scale = 1.0f;
        m_tint = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_shapeResources.Clear();
        m_runtimeStrokeResources.Clear();
        m_sharedShapeAtlas = nullptr;
        m_cachedVertices.Clear();
        m_cachedBatches.Clear();
        m_hasCachedGeometry = false;
        m_hasCachedColor = false;
        m_geometryCacheGeneration = 0;
        m_viewBoxSize = { 0.0f, 0.0f };
        m_localAabb = {};
        m_aabb = {};
        m_estimatedVertexCount = 0;
    }

    void RetainedVectorImageModel::SetOrigin(const Vec2f& origin)
    {
        if ((m_origin.x == origin.x) && (m_origin.y == origin.y))
        {
            return;
        }

        m_origin = origin;
        UpdateAabbFromLocal();
        InvalidateGeometry();
    }

    void RetainedVectorImageModel::SetScale(float scale)
    {
        if (m_scale == scale)
        {
            return;
        }

        m_scale = scale;
        UpdateAabbFromLocal();
        InvalidateGeometry();
    }

    void RetainedVectorImageModel::SetTint(const Vec4f& tint)
    {
        if (EqualColor(m_tint, tint))
        {
            return;
        }

        m_tint = tint;
        InvalidateColor();
    }

    void RetainedVectorImageModel::SetPixelShader(const rhi::Shader* pixelShader)
    {
        if (m_pixelShader == pixelShader)
        {
            return;
        }

        m_pixelShader = pixelShader;
        for (RetainedVectorImageCachedBatch& batch : m_cachedBatches)
        {
            batch.pixelShader = pixelShader;
        }
    }

    FontFaceHandle RetainedVectorImageModel::GetFontFaceHandle(uint32_t fontFaceIndex) const
    {
        if (fontFaceIndex >= m_fontFaces.Size())
        {
            return {};
        }

        return m_fontFaces[fontFaceIndex];
    }

    bool RetainedVectorImageModel::TryGetPreparedShapeResource(
        uint32_t shapeIndex,
        RetainedVectorImageShapeResourceKind resourceKind,
        const StrokeStyle& style,
        Renderer& renderer,
        const GlyphResource*& out) const
    {
        out = nullptr;
        if ((shapeIndex >= m_shapeResources.Size()) || !m_image.IsValid())
        {
            return false;
        }

        Vector<GlyphResource>& resources = (resourceKind == RetainedVectorImageShapeResourceKind::RuntimeRestroke)
            ? const_cast<Vector<GlyphResource>&>(m_runtimeStrokeResources)
            : const_cast<Vector<GlyphResource>&>(m_shapeResources);
        GlyphResource& resource = resources[shapeIndex];
        if (resource.atlas == nullptr)
        {
            if (resourceKind == RetainedVectorImageShapeResourceKind::RuntimeRestroke)
            {
                CompiledStrokedVectorShapeResourceData shapeData{};
                if (!BuildCompiledStrokedVectorShapeResourceData(shapeData, m_image, shapeIndex, style)
                    || !renderer.CreateGlyphResource(resource, shapeData.createInfo))
                {
                    return false;
                }
            }
            else
            {
                CompiledVectorShapeResourceData shapeData{};
                if (!BuildCompiledVectorShapeResourceData(shapeData, m_image, shapeIndex))
                {
                    return false;
                }

                GlyphAtlas*& sharedShapeAtlas = const_cast<GlyphAtlas*&>(m_sharedShapeAtlas);
                if (sharedShapeAtlas == nullptr)
                {
                    if (!renderer.CreateGlyphResource(resource, shapeData.createInfo))
                    {
                        return false;
                    }

                    sharedShapeAtlas = resource.atlas;
                }
                else
                {
                    MemCopy(
                        resource.vertices,
                        shapeData.vertices,
                        shapeData.createInfo.vertexCount * sizeof(PackedGlyphVertex));
                    resource.vertexCount = shapeData.createInfo.vertexCount;
                    resource.atlas = sharedShapeAtlas;
                }
            }
        }

        out = &resource;
        return true;
    }

    void RetainedVectorImageModel::RebuildLocalAabb(Renderer& renderer)
    {
        m_localAabb = {};

        PackedGlyphVertex transformedVertices[ScribeGlyphVertexCount]{};
        for (const RetainedVectorImageDraw& draw : m_draws)
        {
            const GlyphResource* shapeResource = nullptr;
            const bool ok = TryGetPreparedShapeResource(
                draw.shapeIndex,
                GetShapeResourceKind(draw),
                draw.strokeStyle,
                renderer,
                shapeResource);
            if (!ok || (shapeResource == nullptr))
            {
                continue;
            }

            TransformDrawVertices(transformedVertices, BuildLocalShapeDrawDesc(draw, *shapeResource));
            for (uint32_t vertexIndex = 0; vertexIndex < shapeResource->vertexCount; ++vertexIndex)
            {
                ExpandAabb(m_localAabb, { transformedVertices[vertexIndex].pos.x, transformedVertices[vertexIndex].pos.y });
            }
        }

        for (const RetainedTextDraw& draw : m_textDraws)
        {
            const GlyphResource* glyphResource = nullptr;
            const bool ok = (draw.flags & RetainedTextDrawFlagStroke) != 0
                ? m_context->TryGetStrokedGlyphResource(GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, draw.strokeStyle, glyphResource)
                : m_context->TryGetGlyphResource(GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, glyphResource);
            if (!ok || (glyphResource == nullptr))
            {
                continue;
            }

            TransformDrawVertices(transformedVertices, BuildLocalTextDrawDesc(draw, *glyphResource));
            for (uint32_t vertexIndex = 0; vertexIndex < glyphResource->vertexCount; ++vertexIndex)
            {
                ExpandAabb(m_localAabb, { transformedVertices[vertexIndex].pos.x, transformedVertices[vertexIndex].pos.y });
            }
        }
    }

    void RetainedVectorImageModel::UpdateAabbFromLocal()
    {
        if (m_localAabb.IsEmpty())
        {
            m_aabb = {};
            return;
        }

        const float scaledMinX = m_localAabb.min.x * m_scale;
        const float scaledMinY = m_localAabb.min.y * m_scale;
        const float scaledMaxX = m_localAabb.max.x * m_scale;
        const float scaledMaxY = m_localAabb.max.y * m_scale;
        m_aabb.min = {
            m_origin.x + Min(scaledMinX, scaledMaxX),
            m_origin.y + Min(scaledMinY, scaledMaxY)
        };
        m_aabb.max = {
            m_origin.x + Max(scaledMinX, scaledMaxX),
            m_origin.y + Max(scaledMinY, scaledMaxY)
        };
    }

    bool RetainedVectorImageModel::UpdateRenderData(Renderer& renderer) const
    {
        bool preparedAny = false;
        for (const RetainedVectorImageDraw& draw : m_draws)
        {
            const GlyphResource* shapeResource = nullptr;
            const bool ok = TryGetPreparedShapeResource(
                draw.shapeIndex,
                GetShapeResourceKind(draw),
                draw.strokeStyle,
                renderer,
                shapeResource);
            preparedAny |= ok && (shapeResource != nullptr);
        }

        for (const RetainedTextDraw& draw : m_textDraws)
        {
            const GlyphResource* glyphResource = nullptr;
            const bool ok = (draw.flags & RetainedTextDrawFlagStroke) != 0
                ? m_context->TryGetStrokedGlyphResource(GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, draw.strokeStyle, glyphResource)
                : m_context->TryGetGlyphResource(GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, glyphResource);
            preparedAny |= ok && (glyphResource != nullptr);
        }

        if (!preparedAny)
        {
            return false;
        }

        if (!m_hasCachedGeometry)
        {
            m_cachedVertices.Clear();
            m_cachedVertices.Reserve(m_estimatedVertexCount);
            m_cachedBatches.Clear();
            m_cachedBatches.Reserve(GetDrawCount());

            for (const RetainedVectorImageDraw& draw : m_draws)
            {
                const GlyphResource* shapeResource = nullptr;
                const bool ok = TryGetPreparedShapeResource(
                    draw.shapeIndex,
                    GetShapeResourceKind(draw),
                    draw.strokeStyle,
                    renderer,
                    shapeResource);
                if (!ok || (shapeResource == nullptr))
                {
                    continue;
                }

                const DrawGlyphDesc desc = BuildShapeDrawDesc(*this, draw, *shapeResource);
                const uint32_t oldSize = m_cachedVertices.Size();
                m_cachedVertices.Expand(shapeResource->vertexCount, DefaultInit);
                TransformDrawVertices(m_cachedVertices.Data() + oldSize, desc);
                if (!m_cachedBatches.IsEmpty() && (m_cachedBatches.Back().atlas == shapeResource->atlas))
                {
                    m_cachedBatches.Back().vertexCount += shapeResource->vertexCount;
                }
                else
                {
                    RetainedVectorImageCachedBatch& batch = m_cachedBatches.EmplaceBack();
                    batch.atlas = shapeResource->atlas;
                    batch.pixelShader = m_pixelShader;
                    batch.vertexCount = shapeResource->vertexCount;
                }
            }

            for (const RetainedTextDraw& draw : m_textDraws)
            {
                const GlyphResource* glyphResource = nullptr;
                const bool ok = (draw.flags & RetainedTextDrawFlagStroke) != 0
                    ? m_context->TryGetStrokedGlyphResource(GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, draw.strokeStyle, glyphResource)
                    : m_context->TryGetGlyphResource(GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, glyphResource);
                if (!ok || (glyphResource == nullptr))
                {
                    continue;
                }

                const DrawGlyphDesc desc = BuildTextDrawDesc(*this, draw, *glyphResource);
                const uint32_t oldSize = m_cachedVertices.Size();
                m_cachedVertices.Expand(glyphResource->vertexCount, DefaultInit);
                TransformDrawVertices(m_cachedVertices.Data() + oldSize, desc);
                if (!m_cachedBatches.IsEmpty() && (m_cachedBatches.Back().atlas == glyphResource->atlas))
                {
                    m_cachedBatches.Back().vertexCount += glyphResource->vertexCount;
                }
                else
                {
                    RetainedVectorImageCachedBatch& batch = m_cachedBatches.EmplaceBack();
                    batch.atlas = glyphResource->atlas;
                    batch.pixelShader = m_pixelShader;
                    batch.vertexCount = glyphResource->vertexCount;
                }
            }
            m_hasCachedGeometry = true;
            m_hasCachedColor = true;
            ++m_geometryCacheGeneration;
            return true;
        }

        if (!m_hasCachedColor)
        {
            uint32_t vertexOffset = 0;
            for (const RetainedVectorImageDraw& draw : m_draws)
            {
                const GlyphResource* shapeResource = nullptr;
                const bool ok = TryGetPreparedShapeResource(
                    draw.shapeIndex,
                    GetShapeResourceKind(draw),
                    draw.strokeStyle,
                    renderer,
                    shapeResource);
                if (!ok || (shapeResource == nullptr))
                {
                    continue;
                }

                const DrawGlyphDesc desc = BuildShapeDrawDesc(*this, draw, *shapeResource);
                UpdateDrawVertexColors(m_cachedVertices.Data() + vertexOffset, desc);
                vertexOffset += shapeResource->vertexCount;
            }

            for (const RetainedTextDraw& draw : m_textDraws)
            {
                const GlyphResource* glyphResource = nullptr;
                const bool ok = (draw.flags & RetainedTextDrawFlagStroke) != 0
                    ? m_context->TryGetStrokedGlyphResource(GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, draw.strokeStyle, glyphResource)
                    : m_context->TryGetGlyphResource(GetFontFaceHandle(draw.fontFaceIndex), draw.glyphIndex, glyphResource);
                if (!ok || (glyphResource == nullptr))
                {
                    continue;
                }

                const DrawGlyphDesc desc = BuildTextDrawDesc(*this, draw, *glyphResource);
                UpdateDrawVertexColors(m_cachedVertices.Data() + vertexOffset, desc);
                vertexOffset += glyphResource->vertexCount;
            }

            m_hasCachedColor = true;
        }

        return true;
    }

    void RetainedVectorImageModel::ClearTransformedVertexCache() const
    {
        m_cachedVertices.Clear();
        m_cachedBatches.Clear();
        m_hasCachedGeometry = false;
        m_hasCachedColor = false;
    }

    void RetainedVectorImageModel::InvalidateGeometry() const
    {
        ClearTransformedVertexCache();
    }

    void RetainedVectorImageModel::InvalidateColor() const
    {
        m_hasCachedColor = false;
    }

    void VectorImageBuilder::Clear()
    {
        m_drawPaths.Clear();
        m_currentPath.Clear();
        m_fillStyle = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_strokeColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_strokeStyle = { 1.0f, StrokeJoinStyle::Miter, StrokeCapStyle::Butt, 4.0f };
        m_fillRule = FillRule::NonZero;
        m_viewBoxMinX = 0.0f;
        m_viewBoxMinY = 0.0f;
        m_viewBoxWidth = 0.0f;
        m_viewBoxHeight = 0.0f;
        m_flatteningTolerance = 0.25f;
        m_hasViewBox = false;
    }

    void VectorImageBuilder::SetViewBox(float minX, float minY, float width, float height)
    {
        m_viewBoxMinX = minX;
        m_viewBoxMinY = minY;
        m_viewBoxWidth = Max(width, 1.0f);
        m_viewBoxHeight = Max(height, 1.0f);
        m_hasViewBox = true;
    }

    void VectorImageBuilder::ClearViewBox()
    {
        m_viewBoxMinX = 0.0f;
        m_viewBoxMinY = 0.0f;
        m_viewBoxWidth = 0.0f;
        m_viewBoxHeight = 0.0f;
        m_hasViewBox = false;
    }

    void VectorImageBuilder::BeginPath()
    {
        m_currentPath.Clear();
    }

    void VectorImageBuilder::MoveTo(float x, float y)
    {
        PathCommand& command = m_currentPath.EmplaceBack();
        command.type = PathCommandType::MoveTo;
        command.p0 = { x, y };
    }

    void VectorImageBuilder::LineTo(float x, float y)
    {
        PathCommand& command = m_currentPath.EmplaceBack();
        command.type = PathCommandType::LineTo;
        command.p0 = { x, y };
    }

    void VectorImageBuilder::QuadraticCurveTo(float cx, float cy, float x, float y)
    {
        PathCommand& command = m_currentPath.EmplaceBack();
        command.type = PathCommandType::QuadraticCurveTo;
        command.p0 = { cx, cy };
        command.p1 = { x, y };
    }

    void VectorImageBuilder::BezierCurveTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
    {
        PathCommand& command = m_currentPath.EmplaceBack();
        command.type = PathCommandType::BezierCurveTo;
        command.p0 = { c1x, c1y };
        command.p1 = { c2x, c2y };
        command.p2 = { x, y };
    }

    void VectorImageBuilder::ClosePath()
    {
        PathCommand& command = m_currentPath.EmplaceBack();
        command.type = PathCommandType::ClosePath;
    }

    void VectorImageBuilder::Rect(float x, float y, float width, float height)
    {
        if ((width <= 0.0f) || (height <= 0.0f))
        {
            return;
        }

        MoveTo(x, y);
        LineTo(x + width, y);
        LineTo(x + width, y + height);
        LineTo(x, y + height);
        ClosePath();
    }

    void VectorImageBuilder::Ellipse(float cx, float cy, float radiusX, float radiusY)
    {
        if ((radiusX <= 0.0f) || (radiusY <= 0.0f))
        {
            return;
        }

        const float ox = radiusX * RuntimeEllipseKappa;
        const float oy = radiusY * RuntimeEllipseKappa;
        MoveTo(cx + radiusX, cy);
        BezierCurveTo(cx + radiusX, cy + oy, cx + ox, cy + radiusY, cx, cy + radiusY);
        BezierCurveTo(cx - ox, cy + radiusY, cx - radiusX, cy + oy, cx - radiusX, cy);
        BezierCurveTo(cx - radiusX, cy - oy, cx - ox, cy - radiusY, cx, cy - radiusY);
        BezierCurveTo(cx + ox, cy - radiusY, cx + radiusX, cy - oy, cx + radiusX, cy);
        ClosePath();
    }

    void VectorImageBuilder::Line(float x0, float y0, float x1, float y1)
    {
        MoveTo(x0, y0);
        LineTo(x1, y1);
    }

    void VectorImageBuilder::Polyline(Span<const Vec2f> points)
    {
        if (points.Size() < 2u)
        {
            return;
        }

        MoveTo(points[0]);
        for (uint32_t pointIndex = 1; pointIndex < points.Size(); ++pointIndex)
        {
            LineTo(points[pointIndex]);
        }
    }

    void VectorImageBuilder::Polygon(Span<const Vec2f> points)
    {
        if (points.Size() < 3u)
        {
            return;
        }

        Polyline(points);
        ClosePath();
    }

    bool VectorImageBuilder::Fill()
    {
        if (m_currentPath.IsEmpty())
        {
            return false;
        }

        DrawPath& drawPath = m_drawPaths.EmplaceBack();
        drawPath.commands = m_currentPath;
        drawPath.fill = true;
        drawPath.stroke = false;
        drawPath.fillRule = m_fillRule;
        drawPath.fillStyle = m_fillStyle;
        drawPath.strokeColor = m_strokeColor;
        drawPath.strokeStyle = m_strokeStyle;
        return true;
    }

    bool VectorImageBuilder::Stroke()
    {
        if (m_currentPath.IsEmpty())
        {
            return false;
        }

        DrawPath& drawPath = m_drawPaths.EmplaceBack();
        drawPath.commands = m_currentPath;
        drawPath.fill = false;
        drawPath.stroke = true;
        drawPath.fillRule = m_fillRule;
        drawPath.fillStyle = m_fillStyle;
        drawPath.strokeColor = m_strokeColor;
        drawPath.strokeStyle = m_strokeStyle;
        return true;
    }

    bool VectorImageBuilder::Build(Vector<schema::Word>& outWords, VectorImageResourceReader& outImage) const
    {
        outWords.Clear();
        outImage = {};
        if (m_drawPaths.IsEmpty())
        {
            return false;
        }

        editor::CompiledVectorImageData imageData{};
        imageData.bandOverlapEpsilon = RuntimeVectorImageBandOverlapEpsilon;
        imageData.curveTextureWidth = editor::curve_compile::CurveTextureWidth;
        imageData.bandTextureWidth = ScribeBandTextureWidth;

        auto compilePathGeometry = [&](Vector<editor::curve_compile::CurveData>& outCurves,
                                       Vector<editor::StrokeSourcePoint>& outStrokePoints,
                                       Vector<editor::StrokeSourceCommand>& outStrokeCommands,
                                       Span<const PathCommand> commands,
                                       bool closeOpenSubpaths) -> bool
        {
            outCurves.Clear();
            outStrokePoints.Clear();
            outStrokeCommands.Clear();
            if (commands.IsEmpty())
            {
                return false;
            }

            editor::OutlineBuilder builder(m_flatteningTolerance);
            bool hasCurrent = false;
            for (const PathCommand& command : commands)
            {
                switch (command.type)
                {
                    case PathCommandType::MoveTo:
                        if (builder.HasOpenSubpath())
                        {
                            builder.EndOpenSubpath(closeOpenSubpaths);
                        }
                        builder.BeginSubpath({ command.p0.x, command.p0.y });
                        hasCurrent = true;
                        break;

                    case PathCommandType::LineTo:
                        if (!hasCurrent)
                        {
                            builder.BeginSubpath({ command.p0.x, command.p0.y });
                            hasCurrent = true;
                        }
                        else
                        {
                            builder.LineTo({ command.p0.x, command.p0.y });
                        }
                        break;

                    case PathCommandType::QuadraticCurveTo:
                        if (!hasCurrent)
                        {
                            builder.BeginSubpath({ command.p1.x, command.p1.y });
                            hasCurrent = true;
                        }
                        else
                        {
                            builder.QuadraticTo(
                                { command.p0.x, command.p0.y },
                                { command.p1.x, command.p1.y });
                        }
                        break;

                    case PathCommandType::BezierCurveTo:
                        if (!hasCurrent)
                        {
                            builder.BeginSubpath({ command.p2.x, command.p2.y });
                            hasCurrent = true;
                        }
                        else
                        {
                            builder.CubicTo(
                                { command.p0.x, command.p0.y },
                                { command.p1.x, command.p1.y },
                                { command.p2.x, command.p2.y });
                        }
                        break;

                    case PathCommandType::ClosePath:
                        if (builder.HasOpenSubpath())
                        {
                            builder.CloseSubpath(true);
                        }
                        hasCurrent = false;
                        break;
                }
            }

            if (builder.HasOpenSubpath())
            {
                builder.EndOpenSubpath(closeOpenSubpaths);
            }

            outCurves = Move(builder.Curves());
            outStrokePoints = Move(builder.Points());
            outStrokeCommands = Move(builder.Commands());
            return !outCurves.IsEmpty() || !outStrokeCommands.IsEmpty();
        };

        for (const DrawPath& drawPath : m_drawPaths)
        {
            if (drawPath.fill && (drawPath.fillStyle.w > 0.0f))
            {
                Vector<editor::curve_compile::CurveData> fillCurves{};
                Vector<editor::StrokeSourcePoint> fillStrokePoints{};
                Vector<editor::StrokeSourceCommand> fillStrokeCommands{};
                if (!compilePathGeometry(
                        fillCurves,
                        fillStrokePoints,
                        fillStrokeCommands,
                        Span<const PathCommand>(drawPath.commands.Data(), drawPath.commands.Size()),
                        true))
                {
                    continue;
                }

                uint32_t shapeIndex = 0;
                if (!AppendCompiledVectorShape(
                        imageData,
                        Span<const editor::curve_compile::CurveData>(fillCurves.Data(), fillCurves.Size()),
                        Span<const editor::StrokeSourcePoint>(fillStrokePoints.Data(), fillStrokePoints.Size()),
                        Span<const editor::StrokeSourceCommand>(fillStrokeCommands.Data(), fillStrokeCommands.Size()),
                        drawPath.fillRule,
                        shapeIndex))
                {
                    return false;
                }

                editor::CompiledVectorImageLayerEntry& layer = imageData.layers.EmplaceBack();
                layer.shapeIndex = shapeIndex;
                layer.kind = VectorLayerKind::Fill;
                layer.red = drawPath.fillStyle.x;
                layer.green = drawPath.fillStyle.y;
                layer.blue = drawPath.fillStyle.z;
                layer.alpha = drawPath.fillStyle.w;
            }

            if (drawPath.stroke && (drawPath.strokeColor.w > 0.0f) && drawPath.strokeStyle.IsVisible())
            {
                Vector<editor::curve_compile::CurveData> strokePathCurves{};
                Vector<editor::StrokeSourcePoint> strokeSourcePoints{};
                Vector<editor::StrokeSourceCommand> strokeSourceCommands{};
                if (!compilePathGeometry(
                        strokePathCurves,
                        strokeSourcePoints,
                        strokeSourceCommands,
                        Span<const PathCommand>(drawPath.commands.Data(), drawPath.commands.Size()),
                        false))
                {
                    continue;
                }

                Vector<editor::curve_compile::CurveData> strokeCurves{};
                if (!BuildStrokedFillCurves(
                        strokeCurves,
                        Span<const editor::StrokeSourcePoint>(strokeSourcePoints.Data(), strokeSourcePoints.Size()),
                        Span<const editor::StrokeSourceCommand>(strokeSourceCommands.Data(), strokeSourceCommands.Size()),
                        drawPath.strokeStyle,
                        m_flatteningTolerance))
                {
                    continue;
                }

                uint32_t shapeIndex = 0;
                if (!AppendCompiledVectorShape(
                        imageData,
                        Span<const editor::curve_compile::CurveData>(strokeCurves.Data(), strokeCurves.Size()),
                        {},
                        {},
                        FillRule::NonZero,
                        shapeIndex))
                {
                    return false;
                }

                editor::CompiledVectorImageLayerEntry& layer = imageData.layers.EmplaceBack();
                layer.shapeIndex = shapeIndex;
                layer.kind = VectorLayerKind::Stroke;
                layer.red = drawPath.strokeColor.x;
                layer.green = drawPath.strokeColor.y;
                layer.blue = drawPath.strokeColor.z;
                layer.alpha = drawPath.strokeColor.w;
                layer.strokeWidth = drawPath.strokeStyle.width;
                layer.strokeJoin = ToSchemaStrokeJoin(drawPath.strokeStyle.joinStyle);
                layer.strokeCap = ToSchemaStrokeCap(drawPath.strokeStyle.capStyle);
                layer.strokeMiterLimit = drawPath.strokeStyle.miterLimit;
            }
        }

        if (imageData.shapes.IsEmpty() || imageData.layers.IsEmpty())
        {
            return false;
        }

        editor::PadCurveTexture(imageData.curveTexels, imageData.curveTextureWidth, imageData.curveTextureHeight);
        editor::PadBandTexture(imageData.bandTexels, imageData.bandTextureWidth, imageData.bandTextureHeight);

        if (m_hasViewBox)
        {
            imageData.viewBoxMinX = m_viewBoxMinX;
            imageData.viewBoxMinY = m_viewBoxMinY;
            imageData.viewBoxWidth = Max(m_viewBoxWidth, 1.0f);
            imageData.viewBoxHeight = Max(m_viewBoxHeight, 1.0f);
        }
        else
        {
            imageData.viewBoxMinX = imageData.boundsMinX;
            imageData.viewBoxMinY = imageData.boundsMinY;
            imageData.viewBoxWidth = Max(imageData.boundsMaxX - imageData.boundsMinX, 1.0f);
            imageData.viewBoxHeight = Max(imageData.boundsMaxY - imageData.boundsMinY, 1.0f);
        }

        schema::Builder rootBuilder;
        VectorImageResource::Builder root = rootBuilder.AddStruct<VectorImageResource>();
        editor::FillVectorImageResourceMetadata(root.GetMetadata(), imageData);
        editor::FillVectorImageResourceFillData(root.GetFill(), imageData);
        editor::FillVectorImageResourceStrokeData(root.GetStroke(), imageData);
        editor::FillVectorImageResourcePaintData(root.GetPaint(), imageData);
        editor::FillVectorImageResourceTextData(rootBuilder, root.GetText(), imageData);
        root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
        root.GetFill().SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
        rootBuilder.SetRoot(root);

        outWords = Span<const schema::Word>(rootBuilder);
        outImage = schema::ReadRoot<VectorImageResource>(outWords.Data());
        return outImage.IsValid();
    }

    bool VectorImageBuilder::Build(RetainedVectorImageModel& outImage, ScribeContext& context) const
    {
        Vector<schema::Word> words{};
        VectorImageResourceReader image{};
        if (!Build(words, image))
        {
            return false;
        }

        RetainedVectorImageBuildDesc desc{};
        desc.context = &context;
        desc.image = image;
        desc.imageWords = Span<const schema::Word>(words.Data(), words.Size());
        return outImage.Build(desc);
    }
}
