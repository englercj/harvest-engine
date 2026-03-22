// Copyright Chad Engler

#pragma once

#include "he/scribe/renderer.h"
#include "he/scribe/runtime_blob.h"

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

    bool BuildCompiledGlyphResourceData(
        CompiledGlyphResourceData& out,
        const LoadedFontFaceBlob& fontFace,
        uint32_t glyphIndex);

    uint32_t SelectCompiledFontPalette(const LoadedFontFaceBlob& fontFace, bool darkBackgroundPreferred);
    bool GetCompiledColorGlyphLayers(
        Vector<CompiledColorGlyphLayer>& out,
        const LoadedFontFaceBlob& fontFace,
        uint32_t glyphIndex,
        uint32_t paletteIndex,
        const Vec4f& foregroundColor = { 1.0f, 1.0f, 1.0f, 1.0f });
}
