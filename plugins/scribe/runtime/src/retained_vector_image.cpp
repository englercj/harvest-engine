// Copyright Chad Engler

#include "he/scribe/retained_vector_image.h"

#include "he/scribe/compiled_vector_image.h"

namespace he::scribe
{
    bool RetainedVectorImageModel::Build(const RetainedVectorImageBuildDesc& desc)
    {
        Clear();

        if (!desc.image || !desc.image->root.IsValid() || !desc.image->render.IsValid() || !desc.image->paint.IsValid())
        {
            return false;
        }

        m_image = *desc.image;
        m_viewBoxSize = {
            desc.image->metadata.GetSourceViewBoxWidth(),
            desc.image->metadata.GetSourceViewBoxHeight()
        };

        Vector<CompiledVectorImageLayer> layers{};
        if (!GetCompiledVectorImageLayers(layers, *desc.image))
        {
            return false;
        }

        const Vec2f drawOffset{
            -desc.image->metadata.GetSourceViewBoxMinX(),
            desc.image->metadata.GetSourceViewBoxMinY()
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

    const LoadedVectorImageBlob* RetainedVectorImageModel::GetImage() const
    {
        return m_image.root.IsValid() ? &m_image : nullptr;
    }
}
