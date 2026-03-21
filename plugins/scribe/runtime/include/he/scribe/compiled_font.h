// Copyright Chad Engler

#pragma once

#include "he/scribe/renderer.h"
#include "he/scribe/runtime_blob.h"

namespace he::scribe
{
    struct CompiledGlyphResourceData
    {
        PackedGlyphVertex vertices[ScribeGlyphVertexCount]{};
        GlyphResourceCreateInfo createInfo{};
        FontFaceGlyphRenderData::Reader glyph{};
    };

    bool BuildCompiledGlyphResourceData(
        CompiledGlyphResourceData& out,
        const LoadedFontFaceBlob& fontFace,
        uint32_t glyphIndex,
        const Vec4f& color = { 1.0f, 1.0f, 1.0f, 1.0f });
}
