// Copyright Chad Engler

#pragma once

#include "he/scribe/packed_data.h"
#include "he/scribe/runtime_blob.h"

#include "he/core/span.h"
#include "he/core/vector.h"

namespace he::scribe::editor
{
    struct CompiledGlyphRenderEntry
    {
        int32_t advanceX{ 0 };
        int32_t advanceY{ 0 };
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

    struct CompiledFontRenderData
    {
        Vector<PackedCurveTexel> curveTexels{};
        Vector<PackedBandTexel> bandTexels{};
        Vector<CompiledGlyphRenderEntry> glyphs{};
        uint32_t curveTextureWidth{ 0 };
        uint32_t curveTextureHeight{ 0 };
        uint32_t bandTextureWidth{ ScribeBandTextureWidth };
        uint32_t bandTextureHeight{ 0 };
        float bandOverlapEpsilon{ 0.0f };
    };

    bool BuildCompiledFontRenderData(CompiledFontRenderData& out, Span<const uint8_t> sourceBytes, uint32_t faceIndex);
}
