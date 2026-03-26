// Copyright Chad Engler

#pragma once

#include "curve_compile_utils.h"
#include "outline_compile_data.h"

namespace he::scribe::editor
{
    class StrokeSourceBuilder final
    {
    public:
        explicit StrokeSourceBuilder(float cubicTolerance)
            : m_cubicToleranceSq(cubicTolerance * cubicTolerance)
        {
        }

        void Clear()
        {
            m_points.Clear();
            m_commands.Clear();
            m_current = {};
            m_hasCurrent = false;
        }

        void AppendMoveTo(const curve_compile::Point2& point)
        {
            AppendCommand(StrokeCommandType::MoveTo, &point, 1);
            m_current = point;
            m_hasCurrent = true;
        }

        void AppendLineTo(const curve_compile::Point2& point)
        {
            if (!m_hasCurrent)
            {
                AppendMoveTo(point);
                return;
            }

            AppendCommand(StrokeCommandType::LineTo, &point, 1);
            m_current = point;
        }

        void AppendQuadraticTo(const curve_compile::Point2& control, const curve_compile::Point2& point)
        {
            if (!m_hasCurrent)
            {
                AppendMoveTo(point);
                return;
            }

            const curve_compile::Point2 points[] = { control, point };
            AppendCommand(StrokeCommandType::QuadraticTo, points, HE_LENGTH_OF(points));
            m_current = point;
        }

        void AppendCubicTo(
            const curve_compile::Point2& control1,
            const curve_compile::Point2& control2,
            const curve_compile::Point2& point)
        {
            if (!m_hasCurrent)
            {
                AppendMoveTo(point);
                return;
            }

            FlattenCubic(m_current, control1, control2, point, 0);
        }

        void AppendClose()
        {
            StrokeSourceCommand& command = m_commands.EmplaceBack();
            command.type = StrokeCommandType::Close;
            command.firstPoint = m_points.Size();
            m_hasCurrent = false;
        }

        Vector<StrokeSourcePoint>& Points() { return m_points; }
        Vector<StrokeSourceCommand>& Commands() { return m_commands; }

    private:
        void AppendCommand(StrokeCommandType type, const curve_compile::Point2* points, uint32_t pointCount)
        {
            StrokeSourceCommand& command = m_commands.EmplaceBack();
            command.type = type;
            command.firstPoint = m_points.Size();

            for (uint32_t pointIndex = 0; pointIndex < pointCount; ++pointIndex)
            {
                StrokeSourcePoint& point = m_points.EmplaceBack();
                point.x = points[pointIndex].x;
                point.y = points[pointIndex].y;
            }
        }

        void FlattenCubic(
            const curve_compile::Point2& p0,
            const curve_compile::Point2& p1,
            const curve_compile::Point2& p2,
            const curve_compile::Point2& p3,
            uint32_t depth)
        {
            const float d1 = curve_compile::DistanceToLineSq(p1, p0, p3);
            const float d2 = curve_compile::DistanceToLineSq(p2, p0, p3);
            if ((depth >= curve_compile::MaxCubicSubdivisionDepth) || (Max(d1, d2) <= m_cubicToleranceSq))
            {
                AppendLineTo(p3);
                return;
            }

            const curve_compile::Point2 p01 = curve_compile::MidPoint(p0, p1);
            const curve_compile::Point2 p12 = curve_compile::MidPoint(p1, p2);
            const curve_compile::Point2 p23 = curve_compile::MidPoint(p2, p3);
            const curve_compile::Point2 p012 = curve_compile::MidPoint(p01, p12);
            const curve_compile::Point2 p123 = curve_compile::MidPoint(p12, p23);
            const curve_compile::Point2 p0123 = curve_compile::MidPoint(p012, p123);

            FlattenCubic(p0, p01, p012, p0123, depth + 1);
            FlattenCubic(p0123, p123, p23, p3, depth + 1);
        }

    private:
        Vector<StrokeSourcePoint> m_points{};
        Vector<StrokeSourceCommand> m_commands{};
        curve_compile::Point2 m_current{};
        float m_cubicToleranceSq{ 0.0f };
        bool m_hasCurrent{ false };
    };
}
