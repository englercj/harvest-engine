// Copyright Chad Engler

#pragma once

#include "he/scribe/context.h"
#include "he/scribe/packed_data.h"
#include "he/scribe/schema_types.h"

namespace he::scribe
{
    struct StrokedShapeData
    {
        Vector<PackedCurveTexel> curveTexels{};
        Vector<PackedBandTexel> bandTexels{};
        float boundsMinX{ 0.0f };
        float boundsMinY{ 0.0f };
        float boundsMaxX{ 0.0f };
        float boundsMaxY{ 0.0f };
        float bandScaleX{ 0.0f };
        float bandScaleY{ 0.0f };
        float bandOffsetX{ 0.0f };
        float bandOffsetY{ 0.0f };
        uint32_t bandMaxX{ 0 };
        uint32_t bandMaxY{ 0 };
        uint32_t curveTextureWidth{ 0 };
        uint32_t curveTextureHeight{ 0 };
        uint32_t bandTextureWidth{ ScribeBandTextureWidth };
        uint32_t bandTextureHeight{ 0 };
        FillRule fillRule{ FillRule::NonZero };
    };

    bool BuildStrokedShapeData(
        StrokedShapeData& out,
        float pointScale,
        schema::List<StrokePoint>::Reader points,
        schema::List<StrokeCommand>::Reader commands,
        uint32_t firstCommand,
        uint32_t commandCount,
        const StrokeStyle& style);
}
