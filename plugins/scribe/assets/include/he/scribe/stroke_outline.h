// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

#include "he/core/span.h"
#include "he/core/vector.h"

namespace he::scribe
{
    struct StrokeOutlineSourcePoint
    {
        float x{ 0.0f };
        float y{ 0.0f };
    };

    struct StrokeOutlineSourceCommand
    {
        StrokeCommandType type{ StrokeCommandType::MoveTo };
        uint32_t firstPoint{ 0 };
    };

    enum class StrokeOutlineCurveKind : uint8_t
    {
        Line,
        Quadratic,
        Cubic,
    };

    struct StrokeOutlineStyle
    {
        float width{ 0.0f };
        StrokeJoinKind join{ StrokeJoinKind::Miter };
        StrokeCapKind cap{ StrokeCapKind::Butt };
        float miterLimit{ 4.0f };

        [[nodiscard]] bool IsVisible() const
        {
            return (width > 0.0f) && (miterLimit > 0.0f);
        }
    };

    struct StrokeOutlineCurve
    {
        StrokeOutlineCurveKind kind{ StrokeOutlineCurveKind::Line };
        float x0{ 0.0f };
        float y0{ 0.0f };
        float x1{ 0.0f };
        float y1{ 0.0f };
        float x2{ 0.0f };
        float y2{ 0.0f };
        float x3{ 0.0f };
        float y3{ 0.0f };
    };

    bool BuildStrokedOutlineCurves(
        Vector<StrokeOutlineCurve>& out,
        Span<const StrokeOutlineSourcePoint> points,
        Span<const StrokeOutlineSourceCommand> commands,
        const StrokeOutlineStyle& style);

    bool BuildStrokedOutlineCurves(
        Vector<StrokeOutlineCurve>& out,
        float pointScale,
        schema::List<StrokePoint>::Reader points,
        schema::List<StrokeCommand>::Reader commands,
        uint32_t firstCommand,
        uint32_t commandCount,
        const StrokeOutlineStyle& style);
}
