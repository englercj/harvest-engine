// Copyright Chad Engler

#pragma once

#include "he/scribe/packed_data.h"
#include "he/scribe/runtime_blob.h"

#include "he/core/span.h"
#include "he/core/vector.h"
#include "he/math/types.h"

namespace he::scribe::editor
{
    struct CompiledVectorShapeRenderEntry
    {
        float boundsMinX{ 0.0f };
        float boundsMinY{ 0.0f };
        float boundsMaxX{ 0.0f };
        float boundsMaxY{ 0.0f };
        float bandScaleX{ 0.0f };
        float bandScaleY{ 0.0f };
        float bandOffsetX{ 0.0f };
        float bandOffsetY{ 0.0f };
        uint32_t glyphBandLocX{ 0 };
        uint32_t glyphBandLocY{ 0 };
        uint32_t bandMaxX{ 0 };
        uint32_t bandMaxY{ 0 };
        FillRule fillRule{ FillRule::NonZero };
        uint32_t flags{ 0 };
    };

    struct CompiledVectorImageLayerEntry
    {
        uint32_t shapeIndex{ 0 };
        float red{ 1.0f };
        float green{ 1.0f };
        float blue{ 1.0f };
        float alpha{ 1.0f };
    };

    struct CompiledVectorImageData
    {
        Vector<PackedCurveTexel> curveTexels{};
        Vector<PackedBandTexel> bandTexels{};
        Vector<CompiledVectorShapeRenderEntry> shapes{};
        Vector<CompiledVectorImageLayerEntry> layers{};
        uint32_t curveTextureWidth{ 0 };
        uint32_t curveTextureHeight{ 0 };
        uint32_t bandTextureWidth{ ScribeBandTextureWidth };
        uint32_t bandTextureHeight{ 0 };
        float bandOverlapEpsilon{ 0.0f };
        float viewBoxMinX{ 0.0f };
        float viewBoxMinY{ 0.0f };
        float viewBoxWidth{ 0.0f };
        float viewBoxHeight{ 0.0f };
        float boundsMinX{ 0.0f };
        float boundsMinY{ 0.0f };
        float boundsMaxX{ 0.0f };
        float boundsMaxY{ 0.0f };
    };

    bool BuildCompiledVectorImageData(
        CompiledVectorImageData& out,
        Span<const uint8_t> sourceBytes,
        float flatteningTolerance);
}
