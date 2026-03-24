// Copyright Chad Engler

#pragma once

#include "he/scribe/packed_data.h"
#include "he/scribe/renderer.h"
#include "he/scribe/schema_types.h"

#include "he/core/utils.h"

namespace he::scribe
{
    inline float PackCompiledShapeBits(uint32_t value)
    {
        return BitCast<float>(value);
    }

    inline uint32_t BuildCompiledShapeBandInfo(FillRule fillRule, uint32_t bandMaxX, uint32_t bandMaxY)
    {
        uint32_t bandInfo = bandMaxX | (bandMaxY << 16);
        if (fillRule == FillRule::EvenOdd)
        {
            bandInfo |= 0x10000000u;
        }

        return bandInfo;
    }

    inline PackedGlyphVertex MakeCompiledShapeVertex(
        float x,
        float y,
        float nx,
        float ny,
        float u,
        float v,
        float glyphLocBits,
        float bandInfoBits,
        const Vec4f& jacobian,
        const Vec4f& banding,
        const Vec4f& color)
    {
        PackedGlyphVertex vertex{};
        vertex.pos = { x, y, nx, ny };
        vertex.tex = { u, v, glyphLocBits, bandInfoBits };
        vertex.jac = jacobian;
        vertex.bnd = banding;
        vertex.col = color;
        return vertex;
    }

    inline void FillCompiledShapeCreateInfo(
        GlyphResourceCreateInfo& out,
        PackedGlyphVertex* vertices,
        const schema::Blob::Reader& curveBytes,
        uint32_t curveTextureWidth,
        uint32_t curveTextureHeight,
        const schema::Blob::Reader& bandBytes,
        uint32_t bandTextureWidth,
        uint32_t bandTextureHeight)
    {
        out.vertices = vertices;
        out.vertexCount = ScribeGlyphVertexCount;
        out.curveTexture.data = curveBytes.Data();
        out.curveTexture.size = { curveTextureWidth, curveTextureHeight };
        out.curveTexture.rowPitch = curveTextureWidth * sizeof(PackedCurveTexel);
        out.bandTexture.data = bandBytes.Data();
        out.bandTexture.size = { bandTextureWidth, bandTextureHeight };
        out.bandTexture.rowPitch = bandTextureWidth * sizeof(PackedBandTexel);
    }
}
