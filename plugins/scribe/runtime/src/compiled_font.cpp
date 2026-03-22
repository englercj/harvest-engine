// Copyright Chad Engler

#include "he/scribe/compiled_font.h"

#include "he/core/math.h"
#include "he/core/utils.h"

namespace he::scribe
{
    namespace
    {
        constexpr uint32_t PaletteFlagForLightBackground = 0x01u;
        constexpr uint32_t PaletteFlagForDarkBackground = 0x02u;

        float PackBits(uint32_t value)
        {
            return BitCast<float>(value);
        }

        Vec4f GetVertexColor()
        {
            // Base glyph resources stay paint-neutral so the same compiled outline can be
            // instantiated for monochrome text, COLR layers, and future tinted draw paths.
            return { 1.0f, 1.0f, 1.0f, 1.0f };
        }

        Vec4f ToVec4f(FontFacePaletteColor::Reader color)
        {
            return {
                color.GetRed(),
                color.GetGreen(),
                color.GetBlue(),
                color.GetAlpha()
            };
        }

        PackedGlyphVertex MakeVertex(
            float x,
            float y,
            float nx,
            float ny,
            float u,
            float v,
            float glyphLocBits,
            float bandInfoBits,
            const Vec4f& jac,
            const Vec4f& banding)
        {
            PackedGlyphVertex vertex{};
            vertex.pos = { x, y, nx, ny };
            vertex.tex = { u, v, glyphLocBits, bandInfoBits };
            vertex.jac = jac;
            vertex.bnd = banding;
            vertex.col = GetVertexColor();
            return vertex;
        }
    }

    bool BuildCompiledGlyphResourceData(
        CompiledGlyphResourceData& out,
        const LoadedFontFaceBlob& fontFace,
        uint32_t glyphIndex)
    {
        out = {};

        if (!fontFace.render.IsValid())
        {
            return false;
        }

        const auto glyphs = fontFace.render.GetGlyphs();
        if (glyphIndex >= glyphs.Size())
        {
            return false;
        }

        const FontFaceGlyphRenderData::Reader glyph = glyphs[glyphIndex];
        if (!glyph.IsValid() || ((glyph.GetFlags() & CompiledFontGlyphFlagHasGeometry) == 0))
        {
            return false;
        }

        float minX = glyph.GetBoundsMinX();
        float minY = glyph.GetBoundsMinY();
        float maxX = glyph.GetBoundsMaxX();
        float maxY = glyph.GetBoundsMaxY();
        if (maxX <= minX)
        {
            maxX = minX + 1.0f;
        }

        if (maxY <= minY)
        {
            maxY = minY + 1.0f;
        }

        uint32_t bandInfo = glyph.GetBandMaxX() | (glyph.GetBandMaxY() << 16);
        if (glyph.GetFillRule() == FillRule::EvenOdd)
        {
            bandInfo |= 0x10000000u;
        }

        const uint32_t glyphLoc = glyph.GetGlyphBandLocX() | (glyph.GetGlyphBandLocY() << 16);
        const float glyphLocBits = PackBits(glyphLoc);
        const float bandInfoBits = PackBits(bandInfo);
        const Vec4f jacobian{
            1.0f,
            0.0f,
            0.0f,
            -1.0f
        };
        const Vec4f banding{
            glyph.GetBandScaleX(),
            glyph.GetBandScaleY(),
            glyph.GetBandOffsetX(),
            glyph.GetBandOffsetY()
        };

        const float objectMinY = -maxY;
        const float objectMaxY = -minY;

        out.vertices[0] = MakeVertex(minX, objectMinY, -1.0f, -1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding);
        out.vertices[1] = MakeVertex(maxX, objectMinY, 1.0f, -1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding);
        out.vertices[2] = MakeVertex(maxX, objectMaxY, 1.0f, 1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding);
        out.vertices[3] = MakeVertex(minX, objectMinY, -1.0f, -1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding);
        out.vertices[4] = MakeVertex(maxX, objectMaxY, 1.0f, 1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding);
        out.vertices[5] = MakeVertex(minX, objectMaxY, -1.0f, 1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding);

        const auto curveBytes = fontFace.root.GetCurveData();
        const auto bandBytes = fontFace.root.GetBandData();

        out.createInfo.vertices = out.vertices;
        out.createInfo.vertexCount = ScribeGlyphVertexCount;
        out.createInfo.curveTexture.data = curveBytes.Data();
        out.createInfo.curveTexture.size = {
            fontFace.render.GetCurveTextureWidth(),
            fontFace.render.GetCurveTextureHeight()
        };
        out.createInfo.curveTexture.rowPitch = fontFace.render.GetCurveTextureWidth() * sizeof(PackedCurveTexel);
        out.createInfo.bandTexture.data = bandBytes.Data();
        out.createInfo.bandTexture.size = {
            fontFace.render.GetBandTextureWidth(),
            fontFace.render.GetBandTextureHeight()
        };
        out.createInfo.bandTexture.rowPitch = fontFace.render.GetBandTextureWidth() * sizeof(PackedBandTexel);
        out.glyph = glyph;
        return true;
    }

    uint32_t SelectCompiledFontPalette(const LoadedFontFaceBlob& fontFace, bool darkBackgroundPreferred)
    {
        if (!fontFace.paint.IsValid())
        {
            return 0;
        }

        const auto palettes = fontFace.paint.GetPalettes();
        if (palettes.IsEmpty())
        {
            return 0;
        }

        const uint32_t desiredFlag = darkBackgroundPreferred ? PaletteFlagForDarkBackground : PaletteFlagForLightBackground;
        for (uint32_t paletteIndex = 0; paletteIndex < palettes.Size(); ++paletteIndex)
        {
            if ((palettes[paletteIndex].GetFlags() & desiredFlag) != 0)
            {
                return paletteIndex;
            }
        }

        const uint32_t defaultPaletteIndex = fontFace.paint.GetDefaultPaletteIndex();
        return defaultPaletteIndex < palettes.Size() ? defaultPaletteIndex : 0;
    }

    bool GetCompiledColorGlyphLayers(
        Vector<CompiledColorGlyphLayer>& out,
        const LoadedFontFaceBlob& fontFace,
        uint32_t glyphIndex,
        uint32_t paletteIndex,
        const Vec4f& foregroundColor)
    {
        out.Clear();

        if (!fontFace.paint.IsValid())
        {
            return false;
        }

        const auto colorGlyphs = fontFace.paint.GetColorGlyphs();
        if (glyphIndex >= colorGlyphs.Size())
        {
            return true;
        }

        const FontFaceColorGlyph::Reader colorGlyph = colorGlyphs[glyphIndex];
        const uint32_t layerCount = colorGlyph.GetLayerCount();
        if (layerCount == 0)
        {
            return true;
        }

        const auto palettes = fontFace.paint.GetPalettes();
        const auto layers = fontFace.paint.GetLayers();
        if (palettes.IsEmpty())
        {
            return false;
        }

        if (paletteIndex >= palettes.Size())
        {
            paletteIndex = SelectCompiledFontPalette(fontFace, false);
        }

        const FontFacePalette::Reader palette = palettes[paletteIndex];
        const auto colors = palette.GetColors();
        const uint32_t firstLayer = colorGlyph.GetFirstLayer();

        out.Reserve(layerCount);
        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
        {
            const FontFaceColorGlyphLayer::Reader layer = layers[firstLayer + layerIndex];

            CompiledColorGlyphLayer& resolvedLayer = out.EmplaceBack();
            resolvedLayer.glyphIndex = layer.GetGlyphIndex();
            resolvedLayer.color = foregroundColor;
            resolvedLayer.basisX = { layer.GetTransform00(), layer.GetTransform10() };
            resolvedLayer.basisY = { layer.GetTransform01(), layer.GetTransform11() };
            resolvedLayer.offset = { layer.GetTransformTx(), layer.GetTransformTy() };

            if ((layer.GetFlags() & CompiledFontColorLayerFlagUseForeground) == 0)
            {
                const uint32_t paletteEntryIndex = layer.GetPaletteEntryIndex();
                if (paletteEntryIndex < colors.Size())
                {
                    resolvedLayer.color = ToVec4f(colors[paletteEntryIndex]);
                }
            }

            resolvedLayer.color.w *= Max(layer.GetAlphaScale(), 0.0f);
        }

        return true;
    }
}
