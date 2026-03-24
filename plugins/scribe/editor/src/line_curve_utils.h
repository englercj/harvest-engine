// Copyright Chad Engler

#pragma once

#include "he/scribe/packed_data.h"

#include "he/core/math.h"

namespace he::scribe::editor
{
    struct LineCurvePoint
    {
        float x{ 0.0f };
        float y{ 0.0f };
    };

    inline void ComputeLineCurveBounds(
        float& outMinX,
        float& outMinY,
        float& outMaxX,
        float& outMaxY,
        const LineCurvePoint& from,
        const LineCurvePoint& to) noexcept
    {
        outMinX = Min(from.x, to.x);
        outMinY = Min(from.y, to.y);
        outMaxX = Max(from.x, to.x);
        outMaxY = Max(from.y, to.y);
    }

    inline float ComputeMinimalHalfFloatOffset(float value) noexcept
    {
        const uint16_t packedValue = PackFloat16(value);
        float offset = 1.0f / 65536.0f;
        while (offset < 1.0f)
        {
            if (PackFloat16(value + offset) != packedValue)
            {
                return offset;
            }

            offset *= 2.0f;
        }

        return 1.0f;
    }

    inline bool TryComputeStableLineQuadraticControlPoint(
        LineCurvePoint& outControl,
        const LineCurvePoint& from,
        const LineCurvePoint& to,
        float degenerateLineLengthSq) noexcept
    {
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float lenSq = (dx * dx) + (dy * dy);
        if (lenSq <= degenerateLineLengthSq)
        {
            return false;
        }

        const float invLen = 1.0f / Sqrt(lenSq);
        const float len = lenSq * invLen;
        const float midX = Lerp(from.x, to.x, 0.5f);
        const float midY = Lerp(from.y, to.y, 0.5f);
        const float tangentOffset = Min(0.5f, len * 0.25f);
        const float tx = dx * invLen;
        const float ty = dy * invLen;
        constexpr float AxisEpsilon = 1.0e-6f;

        // Keep the control point on the segment so the quadratic remains geometrically exact
        // while avoiding the exact midpoint case that produces unstable line rendering.
        outControl.x = midX + (tx * tangentOffset);
        outControl.y = midY + (ty * tangentOffset);

        // Exact axis-aligned line quadratics can still hit a degenerate root-solver path after
        // half-float packing when a pixel center lands directly on the line. Add the smallest
        // orthogonal offset that survives packing so the runtime stays stable without reintroducing
        // a visible bow.
        if (Abs(dx) <= AxisEpsilon)
        {
            outControl.x += ComputeMinimalHalfFloatOffset(midX);
        }
        else if (Abs(dy) <= AxisEpsilon)
        {
            outControl.y += ComputeMinimalHalfFloatOffset(midY);
        }
        return true;
    }
}
