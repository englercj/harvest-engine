// Copyright Chad Engler

#pragma once

#include "he/scribe/renderer.h"
#include "he/scribe/schema_types.h"

namespace he::scribe
{
    struct CompiledVectorImageLayer
    {
        uint32_t shapeIndex{ 0 };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct CompiledVectorShapeResourceData
    {
        PackedGlyphVertex vertices[ScribeGlyphVertexCount]{};
        GlyphResourceCreateInfo createInfo{};
        VectorImageShapeRenderData::Reader shape{};
    };

    bool BuildCompiledVectorShapeResourceData(
        CompiledVectorShapeResourceData& out,
        const VectorImageResourceReader& image,
        uint32_t shapeIndex);

    bool GetCompiledVectorImageLayers(
        Vector<CompiledVectorImageLayer>& out,
        const VectorImageResourceReader& image);
}
