// Copyright Chad Engler

#include "he/scribe/compiled_font.h"

#include "compiled_shape_utils.h"
#include "stroke_geometry.h"

#include "he/core/math.h"

namespace he::scribe
{
    namespace
    {
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

    }

    bool BuildCompiledGlyphResourceData(
        CompiledGlyphResourceData& out,
        const FontFaceResourceReader& fontFace,
        uint32_t glyphIndex)
    {
        out = {};

        const FontFaceFillData::Reader fill = fontFace.GetFill();
        if (!fill.IsValid())
        {
            return false;
        }

        const schema::List<FontFaceGlyphRenderData>::Reader glyphs = fill.GetGlyphs();
        if (glyphIndex >= glyphs.Size())
        {
            return false;
        }

        const FontFaceGlyphRenderData::Reader glyph = glyphs[glyphIndex];
        if (!glyph.IsValid() || !glyph.GetHasGeometry())
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

        const uint32_t bandInfo = BuildCompiledShapeBandInfo(glyph.GetFillRule(), glyph.GetBandMaxX(), glyph.GetBandMaxY());
        const uint32_t glyphLoc = glyph.GetGlyphBandLocX() | (glyph.GetGlyphBandLocY() << 16);
        const float glyphLocBits = PackCompiledShapeBits(glyphLoc);
        const float bandInfoBits = PackCompiledShapeBits(bandInfo);
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
        const Vec4f color = GetVertexColor();

        out.vertices[0] = MakeCompiledShapeVertex(minX, objectMinY, -1.0f, -1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[1] = MakeCompiledShapeVertex(maxX, objectMinY, 1.0f, -1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[2] = MakeCompiledShapeVertex(maxX, objectMaxY, 1.0f, 1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[3] = MakeCompiledShapeVertex(minX, objectMinY, -1.0f, -1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[4] = MakeCompiledShapeVertex(maxX, objectMaxY, 1.0f, 1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[5] = MakeCompiledShapeVertex(minX, objectMaxY, -1.0f, 1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);

        const schema::Blob::Reader curveBytes = fill.GetCurveData();
        const schema::Blob::Reader bandBytes = fill.GetBandData();

        FillCompiledShapeCreateInfo(
            out.createInfo,
            out.vertices,
            curveBytes,
            fill.GetCurveTextureWidth(),
            fill.GetCurveTextureHeight(),
            bandBytes,
            fill.GetBandTextureWidth(),
            fill.GetBandTextureHeight());
        out.glyph = glyph;
        return true;
    }

    bool BuildCompiledStrokedGlyphResourceData(
        CompiledStrokedGlyphResourceData& out,
        const FontFaceResourceReader& fontFace,
        uint32_t glyphIndex,
        const StrokeStyle& style)
    {
        out = {};

        const FontFaceFillData::Reader fill = fontFace.GetFill();
        const FontFaceStrokeData::Reader stroke = fontFace.GetStroke();
        if (!fill.IsValid() || !stroke.IsValid())
        {
            return false;
        }

        const schema::List<FontFaceGlyphRenderData>::Reader glyphs = fill.GetGlyphs();
        if (glyphIndex >= glyphs.Size())
        {
            return false;
        }

        const FontFaceGlyphRenderData::Reader glyph = glyphs[glyphIndex];
        if (!glyph.IsValid() || (glyph.GetStrokeCommandCount() == 0))
        {
            return false;
        }

        StrokedShapeData shape{};
        if (!BuildStrokedShapeData(
                shape,
                stroke.GetPointScale(),
                stroke.GetPoints(),
                stroke.GetCommands(),
                glyph.GetFirstStrokeCommand(),
                glyph.GetStrokeCommandCount(),
                style))
        {
            return false;
        }

        float minX = shape.boundsMinX;
        float minY = shape.boundsMinY;
        float maxX = shape.boundsMaxX;
        float maxY = shape.boundsMaxY;
        if (maxX <= minX)
        {
            maxX = minX + 1.0f;
        }

        if (maxY <= minY)
        {
            maxY = minY + 1.0f;
        }

        const uint32_t bandInfo = BuildCompiledShapeBandInfo(shape.fillRule, shape.bandMaxX, shape.bandMaxY);
        const float glyphLocBits = PackCompiledShapeBits(0);
        const float bandInfoBits = PackCompiledShapeBits(bandInfo);
        const Vec4f jacobian{ 1.0f, 0.0f, 0.0f, -1.0f };
        const Vec4f banding{
            shape.bandScaleX,
            shape.bandScaleY,
            shape.bandOffsetX,
            shape.bandOffsetY
        };
        const float objectMinY = -maxY;
        const float objectMaxY = -minY;
        const Vec4f color = GetVertexColor();

        out.vertices[0] = MakeCompiledShapeVertex(minX, objectMinY, -1.0f, -1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[1] = MakeCompiledShapeVertex(maxX, objectMinY, 1.0f, -1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[2] = MakeCompiledShapeVertex(maxX, objectMaxY, 1.0f, 1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[3] = MakeCompiledShapeVertex(minX, objectMinY, -1.0f, -1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[4] = MakeCompiledShapeVertex(maxX, objectMaxY, 1.0f, 1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[5] = MakeCompiledShapeVertex(minX, objectMaxY, -1.0f, 1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);

        out.curveTexels = Move(shape.curveTexels);
        out.bandTexels = Move(shape.bandTexels);
        out.createInfo.vertices = out.vertices;
        out.createInfo.vertexCount = ScribeGlyphVertexCount;
        out.createInfo.curveTexture.data = out.curveTexels.Data();
        out.createInfo.curveTexture.size = { shape.curveTextureWidth, shape.curveTextureHeight };
        out.createInfo.curveTexture.rowPitch = shape.curveTextureWidth * sizeof(PackedCurveTexel);
        out.createInfo.bandTexture.data = out.bandTexels.Data();
        out.createInfo.bandTexture.size = { shape.bandTextureWidth, shape.bandTextureHeight };
        out.createInfo.bandTexture.rowPitch = shape.bandTextureWidth * sizeof(PackedBandTexel);
        out.createInfo.vertexColorIsWhite = true;
        return true;
    }

    uint32_t SelectCompiledFontPalette(const FontFaceResourceReader& fontFace, bool darkBackgroundPreferred)
    {
        const FontFacePaintData::Reader paint = fontFace.GetPaint();
        if (!paint.IsValid())
        {
            return 0;
        }

        const schema::List<FontFacePalette>::Reader palettes = paint.GetPalettes();
        if (palettes.IsEmpty())
        {
            return 0;
        }
        for (uint32_t paletteIndex = 0; paletteIndex < palettes.Size(); ++paletteIndex)
        {
            const FontFacePaletteBackground background = palettes[paletteIndex].GetBackground();
            if (background == FontFacePaletteBackground::Any
                || (darkBackgroundPreferred && (background == FontFacePaletteBackground::Dark))
                || (!darkBackgroundPreferred && (background == FontFacePaletteBackground::Light)))
            {
                return paletteIndex;
            }
        }

        const uint32_t defaultPaletteIndex = paint.GetDefaultPaletteIndex();
        return defaultPaletteIndex < palettes.Size() ? defaultPaletteIndex : 0;
    }

    bool GetCompiledColorGlyphLayers(
        Vector<CompiledColorGlyphLayer>& out,
        const FontFaceResourceReader& fontFace,
        uint32_t glyphIndex,
        uint32_t paletteIndex,
        const Vec4f& foregroundColor)
    {
        out.Clear();

        const FontFacePaintData::Reader paint = fontFace.GetPaint();
        if (!paint.IsValid())
        {
            return false;
        }

        const schema::List<FontFaceColorGlyph>::Reader colorGlyphs = paint.GetColorGlyphs();
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

        const schema::List<FontFacePalette>::Reader palettes = paint.GetPalettes();
        const schema::List<FontFaceColorGlyphLayer>::Reader layers = paint.GetLayers();
        if (palettes.IsEmpty())
        {
            return false;
        }

        if (paletteIndex >= palettes.Size())
        {
            paletteIndex = SelectCompiledFontPalette(fontFace, false);
        }

        const FontFacePalette::Reader palette = palettes[paletteIndex];
        const schema::List<FontFacePaletteColor>::Reader colors = palette.GetColors();
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

            if (layer.GetColorSource() != FontFaceColorSource::Foreground)
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
