// Copyright Chad Engler

#pragma once

#include "curve_compile_utils.h"
#include "outline_compile_data.h"

namespace he::scribe::editor
{
    class StrokeSourceBuilder final
    {
    public:
        explicit StrokeSourceBuilder([[maybe_unused]] float cubicTolerance)
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

            const curve_compile::Point2 points[] = { control1, control2, point };
            AppendCommand(StrokeCommandType::CubicTo, points, HE_LENGTH_OF(points));
            m_current = point;
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

    private:
        Vector<StrokeSourcePoint> m_points{};
        Vector<StrokeSourceCommand> m_commands{};
        curve_compile::Point2 m_current{};
        bool m_hasCurrent{ false };
    };
}
