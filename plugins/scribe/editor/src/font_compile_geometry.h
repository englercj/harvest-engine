// Copyright Chad Engler

#pragma once

#include "outline_compile_data.h"

#include "he/scribe/packed_data.h"
#include "he/scribe/schema_types.h"

#include "he/core/span.h"
#include "he/core/vector.h"

namespace he::scribe::editor
{
    struct CompiledFontPaletteColor
    {
        float red{ 0.0f };
        float green{ 0.0f };
        float blue{ 0.0f };
        float alpha{ 1.0f };
    };

    struct CompiledFontPalette
    {
        FontFacePaletteBackground background{ FontFacePaletteBackground::Unspecified };
        Vector<CompiledFontPaletteColor> colors{};
    };

    struct CompiledColorGlyphLayerEntry
    {
        uint32_t glyphIndex{ 0 };
        uint32_t paletteEntryIndex{ 0 };
        FontFaceColorSource colorSource{ FontFaceColorSource::Palette };
        float alphaScale{ 1.0f };
        float transform00{ 1.0f };
        float transform01{ 0.0f };
        float transform10{ 0.0f };
        float transform11{ 1.0f };
        float transformTx{ 0.0f };
        float transformTy{ 0.0f };
    };

    struct CompiledColorGlyphEntry
    {
        uint32_t firstLayer{ 0 };
        uint32_t layerCount{ 0 };
    };

    struct CompiledFontPaintData
    {
        uint32_t defaultPaletteIndex{ 0 };
        Vector<CompiledFontPalette> palettes{};
        Vector<CompiledColorGlyphEntry> colorGlyphs{};
        Vector<CompiledColorGlyphLayerEntry> layers{};
    };

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
        bool hasGeometry{ false };
        bool hasColorLayers{ false };
        uint32_t firstOutlineCommand{ 0 };
        uint32_t outlineCommandCount{ 0 };
    };

    struct CompiledFontRenderData
    {
        Vector<PackedCurveTexel> curveTexels{};
        Vector<PackedBandTexel> bandTexels{};
        Vector<CompiledOutlinePoint> outlinePoints{};
        Vector<CompiledOutlineCommand> outlineCommands{};
        Vector<CompiledGlyphRenderEntry> glyphs{};
        CompiledFontPaintData paint{};
        uint32_t curveTextureWidth{ 0 };
        uint32_t curveTextureHeight{ 0 };
        uint32_t bandTextureWidth{ ScribeBandTextureWidth };
        uint32_t bandTextureHeight{ 0 };
        float bandOverlapEpsilon{ 0.0f };
        uint32_t bandHeaderCount{ 0 };
        uint32_t emittedBandPayloadTexelCount{ 0 };
        uint32_t reusedBandCount{ 0 };
        uint32_t reusedBandPayloadTexelCount{ 0 };
    };

    bool BuildCompiledFontRenderData(CompiledFontRenderData& out, Span<const uint8_t> sourceBytes, uint32_t faceIndex);
}
