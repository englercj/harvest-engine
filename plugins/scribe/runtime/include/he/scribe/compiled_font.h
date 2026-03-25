// Copyright Chad Engler

#pragma once

#include "he/scribe/renderer.h"
#include "he/scribe/schema_types.h"

namespace he::scribe
{
    struct CompiledColorGlyphLayer
    {
        uint32_t glyphIndex{ 0 };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec2f basisX{ 1.0f, 0.0f };
        Vec2f basisY{ 0.0f, 1.0f };
        Vec2f offset{ 0.0f, 0.0f };
    };

    struct CompiledGlyphResourceData
    {
        PackedGlyphVertex vertices[ScribeGlyphVertexCount]{};
        GlyphResourceCreateInfo createInfo{};
        FontFaceGlyphRenderData::Reader glyph{};
    };

    struct CompiledStrokedGlyphResourceData
    {
        PackedGlyphVertex vertices[ScribeGlyphVertexCount]{};
        Vector<PackedCurveTexel> curveTexels{};
        Vector<PackedBandTexel> bandTexels{};
        GlyphResourceCreateInfo createInfo{};
    };

    bool BuildCompiledGlyphResourceData(
        CompiledGlyphResourceData& out,
        const FontFaceResourceReader& fontFace,
        uint32_t glyphIndex);
    bool BuildCompiledStrokedGlyphResourceData(
        CompiledStrokedGlyphResourceData& out,
        const FontFaceResourceReader& fontFace,
        uint32_t glyphIndex,
        const StrokeStyle& style);

    uint32_t SelectCompiledFontPalette(const FontFaceResourceReader& fontFace, bool darkBackgroundPreferred);
    bool GetCompiledColorGlyphLayers(
        Vector<CompiledColorGlyphLayer>& out,
        const FontFaceResourceReader& fontFace,
        uint32_t glyphIndex,
        uint32_t paletteIndex,
        const Vec4f& foregroundColor = { 1.0f, 1.0f, 1.0f, 1.0f });
}
