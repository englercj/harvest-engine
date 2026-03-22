// Copyright Chad Engler

#include "he/scribe/compiled_vector_image.h"

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

        Vec4f GetShapeVertexColor(const LoadedVectorImageBlob& image, uint32_t shapeIndex)
        {
            if (!image.paint.IsValid())
            {
                return { 1.0f, 1.0f, 1.0f, 1.0f };
            }

            const auto layers = image.paint.GetLayers();
            for (uint32_t layerIndex = 0; layerIndex < layers.Size(); ++layerIndex)
            {
                const VectorImageLayer::Reader layer = layers[layerIndex];
                if (layer.GetShapeIndex() == shapeIndex)
                {
                    return {
                        layer.GetRed(),
                        layer.GetGreen(),
                        layer.GetBlue(),
                        layer.GetAlpha()
                    };
                }
            }

            return { 1.0f, 1.0f, 1.0f, 1.0f };
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

    bool BuildCompiledVectorShapeResourceData(
        CompiledVectorShapeResourceData& out,
        const LoadedVectorImageBlob& image,
        uint32_t shapeIndex)
    {
        out = {};

        if (!image.render.IsValid())
        {
            return false;
        }

        const auto shapes = image.render.GetShapes();
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

        uint32_t bandInfo = shape.GetBandMaxX() | (shape.GetBandMaxY() << 16);
        if (shape.GetFillRule() == FillRule::EvenOdd)
        {
            bandInfo |= 0x10000000u;
        }

        const uint32_t glyphLoc = shape.GetGlyphBandLocX() | (shape.GetGlyphBandLocY() << 16);
        const float glyphLocBits = PackBits(glyphLoc);
        const float bandInfoBits = PackBits(bandInfo);
        const Vec4f jacobian{ 1.0f, 0.0f, 0.0f, 1.0f };
        const Vec4f banding{
            shape.GetBandScaleX(),
            shape.GetBandScaleY(),
            shape.GetBandOffsetX(),
            shape.GetBandOffsetY()
        };
        const Vec4f color = GetShapeVertexColor(image, shapeIndex);

        out.vertices[0] = MakeVertex(minX, minY, -1.0f, -1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[1] = MakeVertex(maxX, minY, 1.0f, -1.0f, maxX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[2] = MakeVertex(maxX, maxY, 1.0f, 1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[3] = MakeVertex(minX, minY, -1.0f, -1.0f, minX, minY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[4] = MakeVertex(maxX, maxY, 1.0f, 1.0f, maxX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);
        out.vertices[5] = MakeVertex(minX, maxY, -1.0f, 1.0f, minX, maxY, glyphLocBits, bandInfoBits, jacobian, banding, color);

        const auto curveBytes = image.root.GetCurveData();
        const auto bandBytes = image.root.GetBandData();
        out.createInfo.vertices = out.vertices;
        out.createInfo.vertexCount = ScribeGlyphVertexCount;
        out.createInfo.curveTexture.data = curveBytes.Data();
        out.createInfo.curveTexture.size = {
            image.render.GetCurveTextureWidth(),
            image.render.GetCurveTextureHeight()
        };
        out.createInfo.curveTexture.rowPitch = image.render.GetCurveTextureWidth() * sizeof(PackedCurveTexel);
        out.createInfo.bandTexture.data = bandBytes.Data();
        out.createInfo.bandTexture.size = {
            image.render.GetBandTextureWidth(),
            image.render.GetBandTextureHeight()
        };
        out.createInfo.bandTexture.rowPitch = image.render.GetBandTextureWidth() * sizeof(PackedBandTexel);
        out.shape = shape;
        return true;
    }

    bool GetCompiledVectorImageLayers(
        Vector<CompiledVectorImageLayer>& out,
        const LoadedVectorImageBlob& image)
    {
        out.Clear();
        if (!image.paint.IsValid())
        {
            return false;
        }

        const auto layers = image.paint.GetLayers();
        out.Reserve(layers.Size());
        for (uint32_t layerIndex = 0; layerIndex < layers.Size(); ++layerIndex)
        {
            const VectorImageLayer::Reader layer = layers[layerIndex];
            CompiledVectorImageLayer& resolved = out.EmplaceBack();
            resolved.shapeIndex = layer.GetShapeIndex();
            resolved.color = {
                layer.GetRed(),
                layer.GetGreen(),
                layer.GetBlue(),
                layer.GetAlpha()
            };
        }

        return true;
    }
}
