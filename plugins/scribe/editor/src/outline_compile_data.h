// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

#include "he/core/limits.h"
#include "he/core/math.h"
#include "he/core/span.h"
#include "he/core/vector.h"

namespace he::scribe::editor
{
    inline constexpr float CompiledStrokePointScale = 1.0f / 256.0f;

    struct StrokeSourcePoint
    {
        float x{ 0.0f };
        float y{ 0.0f };
    };

    struct StrokeSourceCommand
    {
        StrokeCommandType type{ StrokeCommandType::MoveTo };
        uint32_t firstPoint{ 0 };
    };

    struct CompiledStrokePoint
    {
        int32_t x{ 0 };
        int32_t y{ 0 };
    };

    struct CompiledStrokeCommand
    {
        StrokeCommandType type{ StrokeCommandType::MoveTo };
        uint32_t firstPoint{ 0 };
    };

    inline int32_t QuantizeStrokeCoord(float value)
    {
        const float scaled = Round(value / CompiledStrokePointScale);
        const float minValue = static_cast<float>(Limits<int32_t>::Min);
        const float maxValue = static_cast<float>(Limits<int32_t>::Max);
        return static_cast<int32_t>(Clamp(scaled, minValue, maxValue));
    }

    inline void AppendCompiledStrokeData(
        Vector<CompiledStrokePoint>& outPoints,
        Vector<CompiledStrokeCommand>& outCommands,
        Span<const StrokeSourcePoint> sourcePoints,
        Span<const StrokeSourceCommand> sourceCommands)
    {
        const uint32_t pointBase = outPoints.Size();
        outPoints.Reserve(outPoints.Size() + sourcePoints.Size());
        for (const StrokeSourcePoint& point : sourcePoints)
        {
            CompiledStrokePoint& compiledPoint = outPoints.EmplaceBack();
            compiledPoint.x = QuantizeStrokeCoord(point.x);
            compiledPoint.y = QuantizeStrokeCoord(point.y);
        }

        outCommands.Reserve(outCommands.Size() + sourceCommands.Size());
        for (const StrokeSourceCommand& sourceCommand : sourceCommands)
        {
            CompiledStrokeCommand& compiledCommand = outCommands.EmplaceBack();
            compiledCommand.type = sourceCommand.type;
            compiledCommand.firstPoint = pointBase + sourceCommand.firstPoint;
        }
    }
}
