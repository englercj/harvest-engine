// Copyright Chad Engler

#pragma once

#include "outline_compile_data.h"

#include "he/scribe/packed_data.h"
#include "he/scribe/schema_types.h"

#include "he/core/span.h"
#include "he/core/vector.h"
#include "he/math/types.h"

namespace he::scribe::editor
{
    struct CompiledVectorShapeRenderEntry
    {
        float boundsMinX{ 0.0f };
        float boundsMinY{ 0.0f };
        float boundsMaxX{ 0.0f };
        float boundsMaxY{ 0.0f };
        float originX{ 0.0f };
        float originY{ 0.0f };
        float bandScaleX{ 0.0f };
        float bandScaleY{ 0.0f };
        float bandOffsetX{ 0.0f };
        float bandOffsetY{ 0.0f };
        uint32_t glyphBandLocX{ 0 };
        uint32_t glyphBandLocY{ 0 };
        uint32_t bandMaxX{ 0 };
        uint32_t bandMaxY{ 0 };
        FillRule fillRule{ FillRule::NonZero };
        uint32_t firstStrokeCommand{ 0 };
        uint32_t strokeCommandCount{ 0 };
    };

    struct CompiledVectorImageLayerEntry
    {
        uint32_t shapeIndex{ 0 };
        VectorLayerKind kind{ VectorLayerKind::Fill };
        float red{ 1.0f };
        float green{ 1.0f };
        float blue{ 1.0f };
        float alpha{ 1.0f };
        float strokeWidth{ 0.0f };
        StrokeJoinKind strokeJoin{ StrokeJoinKind::Miter };
        StrokeCapKind strokeCap{ StrokeCapKind::Butt };
        float strokeMiterLimit{ 4.0f };
    };

    struct CompiledVectorImageTextRunEntry
    {
        uint32_t fontFaceIndex{ 0 };
        ScribeImage::TextAnchorKind anchor{ ScribeImage::TextAnchorKind::Start };
        String text{};
        Vec2f position{ 0.0f, 0.0f };
        float fontSize{ 16.0f };
        Vec4f color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Vec4f strokeColor{ 0.0f, 0.0f, 0.0f, 0.0f };
        Vec2f transformX{ 1.0f, 0.0f };
        Vec2f transformY{ 0.0f, 1.0f };
        Vec2f transformTranslation{ 0.0f, 0.0f };
        float strokeWidth{ 0.0f };
        StrokeJoinKind strokeJoin{ StrokeJoinKind::Round };
        StrokeCapKind strokeCap{ StrokeCapKind::Round };
        float strokeMiterLimit{ 4.0f };
    };

    struct CompiledVectorImageFontFaceEntry
    {
        String key{};
    };

    struct CompiledVectorImageData
    {
        Vector<PackedCurveTexel> curveTexels{};
        Vector<PackedBandTexel> bandTexels{};
        Vector<CompiledStrokePoint> strokePoints{};
        Vector<CompiledStrokeCommand> strokeCommands{};
        Vector<CompiledVectorShapeRenderEntry> shapes{};
        Vector<CompiledVectorImageLayerEntry> layers{};
        Vector<CompiledVectorImageFontFaceEntry> fontFaces{};
        Vector<CompiledVectorImageTextRunEntry> textRuns{};
        uint32_t curveTextureWidth{ 0 };
        uint32_t curveTextureHeight{ 0 };
        uint32_t bandTextureWidth{ ScribeBandTextureWidth };
        uint32_t bandTextureHeight{ 0 };
        float bandOverlapEpsilon{ 0.0f };
        float viewBoxMinX{ 0.0f };
        float viewBoxMinY{ 0.0f };
        float viewBoxWidth{ 0.0f };
        float viewBoxHeight{ 0.0f };
        float boundsMinX{ 0.0f };
        float boundsMinY{ 0.0f };
        float boundsMaxX{ 0.0f };
        float boundsMaxY{ 0.0f };
        uint32_t bandHeaderCount{ 0 };
        uint32_t emittedBandPayloadTexelCount{ 0 };
        uint32_t reusedBandCount{ 0 };
        uint32_t reusedBandPayloadTexelCount{ 0 };
    };

    bool BuildCompiledVectorImageData(
        CompiledVectorImageData& out,
        Span<const uint8_t> sourceBytes,
        float flatteningTolerance);
}
