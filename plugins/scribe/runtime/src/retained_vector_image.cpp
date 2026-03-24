// Copyright Chad Engler

#include "he/scribe/retained_vector_image.h"

#include "he/scribe/compiled_vector_image.h"
#include "resource_key_utils.h"

#include "he/schema/layout.h"

namespace he::scribe
{
    namespace
    {
        VectorImageResourceReader CloneVectorImageResource(Vector<schema::Word>& storage, const VectorImageResourceReader& source)
        {
            schema::Builder builder{};
            VectorImageResource::Builder root = builder.AddStruct<VectorImageResource>();

            const schema::Blob::Reader curveData = source.GetCurveData();
            root.SetCurveData(builder.AddBlob(Span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(curveData.Data()),
                curveData.Size())));

            const schema::Blob::Reader bandData = source.GetBandData();
            root.SetBandData(builder.AddBlob(Span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(bandData.Data()),
                bandData.Size())));

            const VectorImagePaintData::Reader sourcePaint = source.GetPaint();
            VectorImagePaintData::Builder paint = root.GetPaint();
            const schema::List<VectorImageLayer>::Reader sourceLayers = sourcePaint.GetLayers();
            schema::List<VectorImageLayer>::Builder layers = paint.InitLayers(sourceLayers.Size());
            for (uint32_t layerIndex = 0; layerIndex < sourceLayers.Size(); ++layerIndex)
            {
                const VectorImageLayer::Reader sourceLayer = sourceLayers[layerIndex];
                VectorImageLayer::Builder layer = layers[layerIndex];
                layer.SetShapeIndex(sourceLayer.GetShapeIndex());
                layer.SetRed(sourceLayer.GetRed());
                layer.SetGreen(sourceLayer.GetGreen());
                layer.SetBlue(sourceLayer.GetBlue());
                layer.SetAlpha(sourceLayer.GetAlpha());
            }

            const VectorImageRuntimeMetadata::Reader sourceMetadata = source.GetMetadata();
            VectorImageRuntimeMetadata::Builder metadata = root.GetMetadata();
            metadata.SetSourceViewBoxMinX(sourceMetadata.GetSourceViewBoxMinX());
            metadata.SetSourceViewBoxMinY(sourceMetadata.GetSourceViewBoxMinY());
            metadata.SetSourceViewBoxWidth(sourceMetadata.GetSourceViewBoxWidth());
            metadata.SetSourceViewBoxHeight(sourceMetadata.GetSourceViewBoxHeight());
            metadata.SetSourceBoundsMinX(sourceMetadata.GetSourceBoundsMinX());
            metadata.SetSourceBoundsMinY(sourceMetadata.GetSourceBoundsMinY());
            metadata.SetSourceBoundsMaxX(sourceMetadata.GetSourceBoundsMaxX());
            metadata.SetSourceBoundsMaxY(sourceMetadata.GetSourceBoundsMaxY());

            const VectorImageRenderData::Reader sourceRender = source.GetRender();
            VectorImageRenderData::Builder render = root.GetRender();
            render.SetCurveTextureWidth(sourceRender.GetCurveTextureWidth());
            render.SetCurveTextureHeight(sourceRender.GetCurveTextureHeight());
            render.SetBandTextureWidth(sourceRender.GetBandTextureWidth());
            render.SetBandTextureHeight(sourceRender.GetBandTextureHeight());
            render.SetBandOverlapEpsilon(sourceRender.GetBandOverlapEpsilon());

            const schema::List<VectorImageShapeRenderData>::Reader sourceShapes = sourceRender.GetShapes();
            schema::List<VectorImageShapeRenderData>::Builder shapes = render.InitShapes(sourceShapes.Size());
            for (uint32_t shapeIndex = 0; shapeIndex < sourceShapes.Size(); ++shapeIndex)
            {
                const VectorImageShapeRenderData::Reader sourceShape = sourceShapes[shapeIndex];
                VectorImageShapeRenderData::Builder shape = shapes[shapeIndex];
                shape.SetBoundsMinX(sourceShape.GetBoundsMinX());
                shape.SetBoundsMinY(sourceShape.GetBoundsMinY());
                shape.SetBoundsMaxX(sourceShape.GetBoundsMaxX());
                shape.SetBoundsMaxY(sourceShape.GetBoundsMaxY());
                shape.SetBandScaleX(sourceShape.GetBandScaleX());
                shape.SetBandScaleY(sourceShape.GetBandScaleY());
                shape.SetBandOffsetX(sourceShape.GetBandOffsetX());
                shape.SetBandOffsetY(sourceShape.GetBandOffsetY());
                shape.SetGlyphBandLocX(sourceShape.GetGlyphBandLocX());
                shape.SetGlyphBandLocY(sourceShape.GetGlyphBandLocY());
                shape.SetBandMaxX(sourceShape.GetBandMaxX());
                shape.SetBandMaxY(sourceShape.GetBandMaxY());
                shape.SetFillRule(sourceShape.GetFillRule());
            }

            builder.SetRoot(root);
            storage = Span<const schema::Word>(builder);
            return schema::ReadRoot<VectorImageResource>(storage.Data());
        }
    }

    bool RetainedVectorImageModel::Build(const RetainedVectorImageBuildDesc& desc)
    {
        Clear();

        if (!desc.image || !desc.image->IsValid() || !desc.image->GetRender().IsValid() || !desc.image->GetPaint().IsValid())
        {
            return false;
        }

        m_image = CloneVectorImageResource(m_imageStorage, *desc.image);
        m_imageHash = HashVectorImageGeometryResource(m_image);
        const VectorImageRuntimeMetadata::Reader metadata = m_image.GetMetadata();
        m_viewBoxSize = {
            metadata.GetSourceViewBoxWidth(),
            metadata.GetSourceViewBoxHeight()
        };
        const VectorImagePaintData::Reader paint = m_image.GetPaint();
        const schema::List<VectorImageLayer>::Reader layers = paint.GetLayers();
        if (layers.IsEmpty())
        {
            return false;
        }

        const Vec2f drawOffset{
            -metadata.GetSourceViewBoxMinX(),
            metadata.GetSourceViewBoxMinY()
        };

        m_draws.Reserve(layers.Size());
        for (uint32_t layerIndex = 0; layerIndex < layers.Size(); ++layerIndex)
        {
            const VectorImageLayer::Reader layer = layers[layerIndex];
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

        return !m_draws.IsEmpty();
    }

    void RetainedVectorImageModel::Clear()
    {
        m_imageStorage.Clear();
        m_image = {};
        m_draws.Clear();
        m_viewBoxSize = { 0.0f, 0.0f };
        m_imageHash = 0;
        m_estimatedVertexCount = 0;
    }

    const VectorImageResourceReader* RetainedVectorImageModel::GetImage() const
    {
        return m_image.IsValid() ? &m_image : nullptr;
    }
}
