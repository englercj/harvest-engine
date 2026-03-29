// Copyright Chad Engler

#include "he/scribe/retained_vector_image.h"

#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/packed_data.h"
#include "he/scribe/retained_text.h"
#include "he/scribe/renderer.h"

#include "glyph_atlas_utils.h"

#include "he/core/log.h"
#include "he/core/math.h"
#include "he/core/memory_ops.h"

namespace he::scribe
{
    namespace
    {
        bool MatchesCachedInstance(
            const RetainedVectorImageInstanceDesc& a,
            const RetainedVectorImageInstanceDesc& b)
        {
            return (a.origin.x == b.origin.x)
                && (a.origin.y == b.origin.y)
                && (a.scale == b.scale)
                && (a.tint.x == b.tint.x)
                && (a.tint.y == b.tint.y)
                && (a.tint.z == b.tint.z)
                && (a.tint.w == b.tint.w);
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
            metadata.GetSourceViewBoxMinY()
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

        return !m_draws.IsEmpty() || !m_textDraws.IsEmpty();
    }

    void RetainedVectorImageModel::Clear()
    {
        if (m_context)
        {
            GlyphAtlas* const sharedShapeAtlas = m_sharedShapeAtlas;
            bool releasedSharedShapeAtlas = false;
            if (m_context->GetRenderer().IsInitialized())
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
        m_shapeResources.Clear();
        m_runtimeStrokeResources.Clear();
        m_sharedShapeAtlas = nullptr;
        m_cachedVertices.Clear();
        m_cachedBatches.Clear();
        m_cachedInstance = {};
        m_hasCachedInstance = false;
        m_viewBoxSize = { 0.0f, 0.0f };
        m_estimatedVertexCount = 0;
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

    bool RetainedVectorImageModel::HasCachedTransformedVertices(const RetainedVectorImageInstanceDesc& instance) const
    {
        return m_hasCachedInstance
            && MatchesCachedInstance(m_cachedInstance, instance)
            && !m_cachedVertices.IsEmpty()
            && !m_cachedBatches.IsEmpty();
    }

    void RetainedVectorImageModel::SetCachedTransformedVertices(
        const RetainedVectorImageInstanceDesc& instance,
        Vector<PackedGlyphVertex>&& vertices,
        Vector<RetainedVectorImageCachedBatch>&& batches) const
    {
        m_cachedInstance = instance;
        m_cachedVertices = Move(vertices);
        m_cachedBatches = Move(batches);
        m_hasCachedInstance = !m_cachedVertices.IsEmpty() && !m_cachedBatches.IsEmpty();
    }

    void RetainedVectorImageModel::ClearTransformedVertexCache() const
    {
        m_cachedVertices.Clear();
        m_cachedBatches.Clear();
        m_cachedInstance = {};
        m_hasCachedInstance = false;
    }
}
