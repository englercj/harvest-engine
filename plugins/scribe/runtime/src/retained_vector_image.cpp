// Copyright Chad Engler

#include "he/scribe/retained_vector_image.h"

#include "he/scribe/packed_data.h"
#include "he/scribe/retained_text.h"

#include "he/core/math.h"

namespace he::scribe
{
    namespace
    {
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
            const RetainedTextModel& model,
            const ScribeImage::TextRun::Reader& run,
            const Vec2f& drawOffset,
            float anchorOffsetX,
            uint32_t fontFaceIndex)
        {
            for (const RetainedTextDraw& sourceDraw : model.GetDraws())
            {
                RetainedTextDraw& draw = out.EmplaceBack(sourceDraw);
                draw.fontFaceIndex = fontFaceIndex;

                const Vec2f localPosition{
                    run.GetPositionX() + anchorOffsetX + sourceDraw.position.x + (sourceDraw.size.x * sourceDraw.offset.x),
                    run.GetPositionY() + sourceDraw.position.y + (sourceDraw.size.y * sourceDraw.offset.y)
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

        const VectorImageResourceReader image = desc.context->GetVectorImage(desc.image);
        if (!image.IsValid() || !image.GetFill().IsValid() || !image.GetPaint().IsValid())
        {
            return false;
        }

        m_context = desc.context;
        m_image = desc.image;

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
        if (layers.IsEmpty() && textRuns.IsEmpty())
        {
            return false;
        }

        m_fontFaces.Reserve(fontFaces.Size());
        for (uint32_t fontIndex = 0; fontIndex < fontFaces.Size(); ++fontIndex)
        {
            FontFaceHandle handle{};
            if (!desc.context->TryFindFontFaceByAlias(fontFaces[fontIndex].AsView(), handle))
            {
                return false;
            }

            if (!handle.IsValid())
            {
                return false;
            }

            m_fontFaces.PushBack(handle);
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
                stroke.flags = RetainedVectorImageDrawFlagStroke | RetainedVectorImageDrawFlagUseCompiledShape;
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
                stroke.flags = RetainedVectorImageDrawFlagStroke;
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
                return false;
            }

            const StringView runText = srcRun.GetText().AsView();
            if (runText.IsEmpty())
            {
                continue;
            }

            const float fontSize = Max(srcRun.GetFontSize(), 0.01f);
            const FontFaceHandle runFontFace = m_fontFaces[srcRun.GetFontFaceIndex()];

            TextStyle style{};
            style.fontFaceIndex = 0;
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
            layoutDesc.fontFaces = Span<const FontFaceHandle>(&runFontFace, 1);
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
            textBuildDesc.fontFaces = Span<const FontFaceHandle>(&runFontFace, 1);
            textBuildDesc.layout = &layout;
            textBuildDesc.fontSize = fontSize;
            textBuildDesc.styles = Span<const TextStyle>(&style, 1);

            RetainedTextModel model{};
            if (!model.Build(textBuildDesc))
            {
                return false;
            }

            const float anchorOffsetX = GetTextAnchorOffsetX(srcRun.GetAnchor(), layout.width);
            AppendTransformedTextRunDraws(
                m_textDraws,
                m_estimatedVertexCount,
                model,
                srcRun,
                drawOffset,
                anchorOffsetX,
                srcRun.GetFontFaceIndex());
        }

        return !m_draws.IsEmpty() || !m_textDraws.IsEmpty();
    }

    void RetainedVectorImageModel::Clear()
    {
        m_context = nullptr;
        m_image = {};
        m_fontFaces.Clear();
        m_draws.Clear();
        m_textDraws.Clear();
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
}
