// Copyright Chad Engler

#include "he/scribe/retained_vector_image.h"

#include "he/scribe/packed_data.h"

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
        const schema::List<VectorImageLayer>::Reader layers = paint.GetLayers();
        if (layers.IsEmpty())
        {
            return false;
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
            const bool isStrokeLayer = layer.GetKind() == VectorLayerKind::Stroke;
            if (isStrokeLayer)
            {
                RetainedVectorImageDraw& stroke = m_draws.EmplaceBack();
                stroke.shapeIndex = layer.GetShapeIndex();
                stroke.flags = RetainedVectorImageDrawFlagStroke;
                stroke.color = {
                    layer.GetRed(),
                    layer.GetGreen(),
                    layer.GetBlue(),
                    layer.GetAlpha()
                };
                stroke.offset = drawOffset;
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
                stroke.offset = drawOffset;
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
                draw.offset = drawOffset;
                m_estimatedVertexCount += ScribeGlyphVertexCount;
            }
        }

        return !m_draws.IsEmpty();
    }

    void RetainedVectorImageModel::Clear()
    {
        m_context = nullptr;
        m_image = {};
        m_draws.Clear();
        m_viewBoxSize = { 0.0f, 0.0f };
        m_estimatedVertexCount = 0;
    }
}
