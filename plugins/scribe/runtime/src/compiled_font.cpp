// Copyright Chad Engler

#include "he/scribe/compiled_font.h"

#include "he/core/math.h"
#include "he/core/utils.h"

namespace he::scribe
{
    namespace
    {
        float PackBits(uint32_t value)
        {
            return BitCast<float>(value);
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
            const Vec4f& banding,
            const Vec4f& color)
        {
            PackedGlyphVertex vertex{};
            vertex.pos = { x, y, nx, ny };
            vertex.tex = { u, v, glyphLocBits, bandInfoBits };
            vertex.jac = jac;
            vertex.bnd = banding;
            vertex.col = color;
            return vertex;
        }
    }

    bool BuildCompiledGlyphResourceData(
        CompiledGlyphResourceData& out,
        const LoadedFontFaceBlob& fontFace,
        uint32_t glyphIndex,
        const Vec4f& color)
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

        out.vertices[0] = MakeVertex(minX, objectMinY, -1.0f, -1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[1] = MakeVertex(maxX, objectMinY, 1.0f, -1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[2] = MakeVertex(maxX, objectMaxY, 1.0f, 1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[3] = MakeVertex(minX, objectMinY, -1.0f, -1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[4] = MakeVertex(maxX, objectMaxY, 1.0f, 1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[5] = MakeVertex(minX, objectMaxY, -1.0f, 1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);

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
}
