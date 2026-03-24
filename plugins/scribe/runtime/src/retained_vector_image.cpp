// Copyright Chad Engler

#include "he/scribe/retained_vector_image.h"

#include "he/scribe/compiled_vector_image.h"

namespace he::scribe
{
    bool RetainedVectorImageModel::Build(const RetainedVectorImageBuildDesc& desc)
    {
        Clear();

        if (!desc.image || !desc.image->IsValid() || !desc.image->GetRender().IsValid() || !desc.image->GetPaint().IsValid())
        {
            return false;
        }

        m_image = *desc.image;
        const VectorImageRuntimeMetadata::Reader metadata = desc.image->GetMetadata();
        m_viewBoxSize = {
            metadata.GetSourceViewBoxWidth(),
            metadata.GetSourceViewBoxHeight()
        };

        Vector<CompiledVectorImageLayer> layers{};
        if (!GetCompiledVectorImageLayers(layers, *desc.image))
        {
            return false;
        }

        const Vec2f drawOffset{
            -metadata.GetSourceViewBoxMinX(),
            metadata.GetSourceViewBoxMinY()
        };

        m_draws.Reserve(layers.Size());
        for (const CompiledVectorImageLayer& layer : layers)
        {
            RetainedVectorImageDraw& draw = m_draws.EmplaceBack();
            draw.shapeIndex = layer.shapeIndex;
            draw.color = layer.color;
            draw.offset = drawOffset;
            m_estimatedVertexCount += ScribeGlyphVertexCount;
        }

        return !m_draws.IsEmpty();
    }

    void RetainedVectorImageModel::Clear()
    {
        m_image = {};
        m_draws.Clear();
        m_viewBoxSize = { 0.0f, 0.0f };
        m_estimatedVertexCount = 0;
    }

    const VectorImageResourceReader* RetainedVectorImageModel::GetImage() const
    {
        return m_image.IsValid() ? &m_image : nullptr;
    }
}
