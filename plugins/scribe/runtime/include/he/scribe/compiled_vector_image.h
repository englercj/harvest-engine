// Copyright Chad Engler

#pragma once

#include "he/scribe/renderer.h"
#include "he/scribe/schema_types.h"

namespace he::scribe
{
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
}
