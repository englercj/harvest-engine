// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

#include "he/core/hash.h"

namespace he::scribe
{
    inline uint64_t HashFontFaceGeometryResource(const FontFaceResourceReader& fontFace)
    {
        Hash<WyHash> hash{};

        const schema::Blob::Reader curveData = fontFace.GetCurveData();
        hash.Update(curveData.Data(), curveData.Size());

        const schema::Blob::Reader bandData = fontFace.GetBandData();
        hash.Update(bandData.Data(), bandData.Size());

        const FontFaceRenderData::Reader render = fontFace.GetRender();
        hash.Update(render.GetCurveTextureWidth());
        hash.Update(render.GetCurveTextureHeight());
        hash.Update(render.GetBandTextureWidth());
        hash.Update(render.GetBandTextureHeight());
        hash.Update(render.GetBandOverlapEpsilon());

        const schema::List<FontFaceGlyphRenderData>::Reader glyphs = render.GetGlyphs();
        hash.Update(glyphs.Size());
        for (uint32_t glyphIndex = 0; glyphIndex < glyphs.Size(); ++glyphIndex)
        {
            const FontFaceGlyphRenderData::Reader glyph = glyphs[glyphIndex];
            hash.Update(glyph.GetAdvanceX());
            hash.Update(glyph.GetAdvanceY());
            hash.Update(glyph.GetBoundsMinX());
            hash.Update(glyph.GetBoundsMinY());
            hash.Update(glyph.GetBoundsMaxX());
            hash.Update(glyph.GetBoundsMaxY());
            hash.Update(glyph.GetBandScaleX());
            hash.Update(glyph.GetBandScaleY());
            hash.Update(glyph.GetBandOffsetX());
            hash.Update(glyph.GetBandOffsetY());
            hash.Update(glyph.GetGlyphBandLocX());
            hash.Update(glyph.GetGlyphBandLocY());
            hash.Update(glyph.GetBandMaxX());
            hash.Update(glyph.GetBandMaxY());
            hash.Update(static_cast<uint32_t>(glyph.GetFillRule()));
            hash.Update(glyph.GetHasGeometry());
            hash.Update(glyph.GetHasColorLayers());
        }

        return hash.Final();
    }

    inline uint64_t HashVectorImageGeometryResource(const VectorImageResourceReader& image)
    {
        Hash<WyHash> hash{};

        const schema::Blob::Reader curveData = image.GetCurveData();
        hash.Update(curveData.Data(), curveData.Size());

        const schema::Blob::Reader bandData = image.GetBandData();
        hash.Update(bandData.Data(), bandData.Size());

        const VectorImageRenderData::Reader render = image.GetRender();
        hash.Update(render.GetCurveTextureWidth());
        hash.Update(render.GetCurveTextureHeight());
        hash.Update(render.GetBandTextureWidth());
        hash.Update(render.GetBandTextureHeight());
        hash.Update(render.GetBandOverlapEpsilon());

        const schema::List<VectorImageShapeRenderData>::Reader shapes = render.GetShapes();
        hash.Update(shapes.Size());
        for (uint32_t shapeIndex = 0; shapeIndex < shapes.Size(); ++shapeIndex)
        {
            const VectorImageShapeRenderData::Reader shape = shapes[shapeIndex];
            hash.Update(shape.GetBoundsMinX());
            hash.Update(shape.GetBoundsMinY());
            hash.Update(shape.GetBoundsMaxX());
            hash.Update(shape.GetBoundsMaxY());
            hash.Update(shape.GetBandScaleX());
            hash.Update(shape.GetBandScaleY());
            hash.Update(shape.GetBandOffsetX());
            hash.Update(shape.GetBandOffsetY());
            hash.Update(shape.GetGlyphBandLocX());
            hash.Update(shape.GetGlyphBandLocY());
            hash.Update(shape.GetBandMaxX());
            hash.Update(shape.GetBandMaxY());
            hash.Update(static_cast<uint32_t>(shape.GetFillRule()));
        }

        return hash.Final();
    }
}
