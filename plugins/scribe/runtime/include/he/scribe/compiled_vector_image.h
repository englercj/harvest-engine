// Copyright Chad Engler

#pragma once

#include "he/scribe/renderer.h"
#include "he/scribe/runtime_blob.h"

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
        const LoadedVectorImageBlob& image,
        uint32_t shapeIndex);

    bool GetCompiledVectorImageLayers(
        Vector<CompiledVectorImageLayer>& out,
        const LoadedVectorImageBlob& image);
}
