// Copyright Chad Engler

#include "he/scribe/compiled_vector_image.h"

#include "compiled_shape_utils.h"
#include "stroke_geometry.h"

#include "he/core/math.h"

namespace he::scribe
{
    bool BuildCompiledVectorShapeResourceData(
        CompiledVectorShapeResourceData& out,
        const VectorImageResourceReader& image,
        uint32_t shapeIndex)
    {
        out = {};

        const VectorImageRenderData::Reader render = image.GetRender();
        if (!render.IsValid())
        {
            return false;
        }

        const schema::List<VectorImageShapeRenderData>::Reader shapes = render.GetShapes();
        if (shapeIndex >= shapes.Size())
        {
            return false;
        }

        const VectorImageShapeRenderData::Reader shape = shapes[shapeIndex];
        if (!shape.IsValid())
        {
            return false;
        }

        float minX = shape.GetBoundsMinX();
        float minY = shape.GetBoundsMinY();
        float maxX = shape.GetBoundsMaxX();
        float maxY = shape.GetBoundsMaxY();
        if (maxX <= minX)
        {
            maxX = minX + 1.0f;
        }

        if (maxY <= minY)
        {
            maxY = minY + 1.0f;
        }

        const uint32_t bandInfo = BuildCompiledShapeBandInfo(shape.GetFillRule(), shape.GetBandMaxX(), shape.GetBandMaxY());
        const uint32_t glyphLoc = shape.GetGlyphBandLocX() | (shape.GetGlyphBandLocY() << 16);
        const float glyphLocBits = PackCompiledShapeBits(glyphLoc);
        const float bandInfoBits = PackCompiledShapeBits(bandInfo);
        const Vec4f jacobian{ 1.0f, 0.0f, 0.0f, 1.0f };
        const Vec4f banding{
            shape.GetBandScaleX(),
            shape.GetBandScaleY(),
            shape.GetBandOffsetX(),
            shape.GetBandOffsetY()
        };
        const Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };

        out.vertices[0] = MakeCompiledShapeVertex(minX, minY, -1.0f, -1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[1] = MakeCompiledShapeVertex(maxX, minY, 1.0f, -1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[2] = MakeCompiledShapeVertex(maxX, maxY, 1.0f, 1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[3] = MakeCompiledShapeVertex(minX, minY, -1.0f, -1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[4] = MakeCompiledShapeVertex(maxX, maxY, 1.0f, 1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[5] = MakeCompiledShapeVertex(minX, maxY, -1.0f, 1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);

        const schema::Blob::Reader curveBytes = image.GetCurveData();
        const schema::Blob::Reader bandBytes = image.GetBandData();
        FillCompiledShapeCreateInfo(
            out.createInfo,
            out.vertices,
            curveBytes,
            render.GetCurveTextureWidth(),
            render.GetCurveTextureHeight(),
            bandBytes,
            render.GetBandTextureWidth(),
            render.GetBandTextureHeight());
        out.shape = shape;
        return true;
    }

    bool BuildCompiledStrokedVectorShapeResourceData(
        CompiledStrokedVectorShapeResourceData& out,
        const VectorImageResourceReader& image,
        uint32_t shapeIndex,
        const StrokeStyle& style)
    {
        out = {};

        const VectorImageRenderData::Reader render = image.GetRender();
        const VectorImageOutlineData::Reader outline = image.GetOutline();
        if (!render.IsValid() || !outline.IsValid())
        {
            return false;
        }

        const schema::List<VectorImageShapeRenderData>::Reader shapes = render.GetShapes();
        if (shapeIndex >= shapes.Size())
        {
            return false;
        }

        const VectorImageShapeRenderData::Reader shape = shapes[shapeIndex];
        if (!shape.IsValid() || (shape.GetOutlineCommandCount() == 0))
        {
            return false;
        }

        StrokedShapeData strokeShape{};
        if (!BuildStrokedShapeData(
                strokeShape,
                outline.GetPoints(),
                outline.GetCommands(),
                shape.GetFirstOutlineCommand(),
                shape.GetOutlineCommandCount(),
                style))
        {
            return false;
        }

        float minX = strokeShape.boundsMinX;
        float minY = strokeShape.boundsMinY;
        float maxX = strokeShape.boundsMaxX;
        float maxY = strokeShape.boundsMaxY;
        if (maxX <= minX)
        {
            maxX = minX + 1.0f;
        }

        if (maxY <= minY)
        {
            maxY = minY + 1.0f;
        }

        const uint32_t bandInfo = BuildCompiledShapeBandInfo(strokeShape.fillRule, strokeShape.bandMaxX, strokeShape.bandMaxY);
        const float glyphLocBits = PackCompiledShapeBits(0);
        const float bandInfoBits = PackCompiledShapeBits(bandInfo);
        const Vec4f jacobian{ 1.0f, 0.0f, 0.0f, 1.0f };
        const Vec4f banding{
            strokeShape.bandScaleX,
            strokeShape.bandScaleY,
            strokeShape.bandOffsetX,
            strokeShape.bandOffsetY
        };
        const Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };

        out.vertices[0] = MakeCompiledShapeVertex(minX, minY, -1.0f, -1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[1] = MakeCompiledShapeVertex(maxX, minY, 1.0f, -1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[2] = MakeCompiledShapeVertex(maxX, maxY, 1.0f, 1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[3] = MakeCompiledShapeVertex(minX, minY, -1.0f, -1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[4] = MakeCompiledShapeVertex(maxX, maxY, 1.0f, 1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[5] = MakeCompiledShapeVertex(minX, maxY, -1.0f, 1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);

        out.curveTexels = Move(strokeShape.curveTexels);
        out.bandTexels = Move(strokeShape.bandTexels);
        out.createInfo.vertices = out.vertices;
        out.createInfo.vertexCount = ScribeGlyphVertexCount;
        out.createInfo.curveTexture.data = out.curveTexels.Data();
        out.createInfo.curveTexture.size = { strokeShape.curveTextureWidth, strokeShape.curveTextureHeight };
        out.createInfo.curveTexture.rowPitch = strokeShape.curveTextureWidth * sizeof(PackedCurveTexel);
        out.createInfo.bandTexture.data = out.bandTexels.Data();
        out.createInfo.bandTexture.size = { strokeShape.bandTextureWidth, strokeShape.bandTextureHeight };
        out.createInfo.bandTexture.rowPitch = strokeShape.bandTextureWidth * sizeof(PackedBandTexel);
        return true;
    }
}
