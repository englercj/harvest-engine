// Copyright Chad Engler

#pragma once

#include "curve_compile_utils.h"
#include "stroke_compile_utils.h"

#include "he/core/math.h"
#include "he/core/span.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

namespace he::scribe::editor
{
    class OutlineBuilder final
    {
    public:
        explicit OutlineBuilder(float flatteningTolerance)
            : m_curveBuilder(flatteningTolerance)
            , m_strokeBuilder(flatteningTolerance)
        {
        }

        void Clear()
        {
            m_curveBuilder.Clear();
            m_strokeBuilder.Clear();
            m_strokeTransform = {};
            m_current = {};
            m_subpathStart = {};
            m_hasCurrentPoint = false;
            m_hasOpenSubpath = false;
        }

        void SetCurveTransform(const curve_compile::Affine2D& transform)
        {
            m_curveBuilder.SetTransform(transform);
        }

        void SetStrokeTransform(const curve_compile::Affine2D& transform)
        {
            m_strokeTransform = transform;
        }

        void BeginSubpath(const curve_compile::Point2& point)
        {
            m_strokeBuilder.AppendMoveTo(curve_compile::TransformPoint(m_strokeTransform, point));
            m_current = point;
            m_subpathStart = point;
            m_hasCurrentPoint = true;
            m_hasOpenSubpath = true;
        }

        void LineTo(const curve_compile::Point2& point)
        {
            if (!m_hasCurrentPoint)
            {
                BeginSubpath(point);
                return;
            }

            m_curveBuilder.AddLine(m_current, point);
            m_strokeBuilder.AppendLineTo(curve_compile::TransformPoint(m_strokeTransform, point));
            m_current = point;
            m_hasCurrentPoint = true;
        }

        void QuadraticTo(const curve_compile::Point2& control, const curve_compile::Point2& point)
        {
            if (!m_hasCurrentPoint)
            {
                BeginSubpath(point);
                return;
            }

            m_curveBuilder.AddQuadratic(m_current, control, point);
            m_strokeBuilder.AppendQuadraticTo(
                curve_compile::TransformPoint(m_strokeTransform, control),
                curve_compile::TransformPoint(m_strokeTransform, point));
            m_current = point;
            m_hasCurrentPoint = true;
        }

        void CubicTo(
            const curve_compile::Point2& control1,
            const curve_compile::Point2& control2,
            const curve_compile::Point2& point)
        {
            if (!m_hasCurrentPoint)
            {
                BeginSubpath(point);
                return;
            }

            m_curveBuilder.AddCubic(m_current, control1, control2, point);
            m_strokeBuilder.AppendCubicTo(
                curve_compile::TransformPoint(m_strokeTransform, control1),
                curve_compile::TransformPoint(m_strokeTransform, control2),
                curve_compile::TransformPoint(m_strokeTransform, point));
            m_current = point;
            m_hasCurrentPoint = true;
        }

        void EndOpenSubpath(bool emitClosingLine)
        {
            if (!m_hasOpenSubpath)
            {
                return;
            }

            if (emitClosingLine)
            {
                m_curveBuilder.AddLine(m_current, m_subpathStart);
                m_current = m_subpathStart;
            }

            m_hasOpenSubpath = false;
        }

        void CloseSubpath(bool emitClosingLine)
        {
            if (!m_hasOpenSubpath)
            {
                return;
            }

            if (emitClosingLine)
            {
                m_curveBuilder.AddLine(m_current, m_subpathStart);
            }

            m_strokeBuilder.AppendClose();
            m_current = m_subpathStart;
            m_hasCurrentPoint = true;
            m_hasOpenSubpath = false;
        }

        bool HasOpenSubpath() const { return m_hasOpenSubpath; }

        Vector<curve_compile::CurveData>& Curves() { return m_curveBuilder.Curves(); }
        Vector<StrokeSourcePoint>& Points() { return m_strokeBuilder.Points(); }
        Vector<StrokeSourceCommand>& Commands() { return m_strokeBuilder.Commands(); }

    private:
        curve_compile::CurveBuilder m_curveBuilder;
        StrokeSourceBuilder m_strokeBuilder;
        curve_compile::Affine2D m_strokeTransform{};
        curve_compile::Point2 m_current{};
        curve_compile::Point2 m_subpathStart{};
        bool m_hasCurrentPoint{ false };
        bool m_hasOpenSubpath{ false };
    };

    inline void BuildLineOnlyCurves(
        Vector<curve_compile::CurveData>& outCurves,
        Span<const curve_compile::Point2> points,
        bool closed,
        float flatteningTolerance)
    {
        outCurves.Clear();
        if (points.Size() < (closed ? 3u : 2u))
        {
            return;
        }

        curve_compile::CurveBuilder builder(flatteningTolerance);
        curve_compile::Point2 previous = points[0];
        for (uint32_t pointIndex = 1; pointIndex < points.Size(); ++pointIndex)
        {
            const curve_compile::Point2 current = points[pointIndex];
            builder.AddLine(previous, current);
            previous = current;
        }

        if (closed)
        {
            builder.AddLine(previous, points[0]);
        }

        outCurves = Move(builder.Curves());
    }

    inline void BuildLineOnlyStrokeSource(
        Vector<StrokeSourcePoint>& outPoints,
        Vector<StrokeSourceCommand>& outCommands,
        Span<const curve_compile::Point2> points,
        bool closed,
        float flatteningTolerance)
    {
        outPoints.Clear();
        outCommands.Clear();
        if (points.IsEmpty())
        {
            return;
        }

        StrokeSourceBuilder builder(flatteningTolerance);
        builder.AppendMoveTo(points[0]);
        for (uint32_t pointIndex = 1; pointIndex < points.Size(); ++pointIndex)
        {
            builder.AppendLineTo(points[pointIndex]);
        }

        if (closed)
        {
            builder.AppendClose();
        }

        outPoints = Move(builder.Points());
        outCommands = Move(builder.Commands());
    }

    inline bool TryExtractSingleClosedLineContour(
        Vector<curve_compile::Point2>& outPolygon,
        Span<const StrokeSourcePoint> points,
        Span<const StrokeSourceCommand> commands)
    {
        outPolygon.Clear();
        outPolygon.Reserve(commands.Size());

        bool hasContour = false;
        bool closed = false;
        for (const StrokeSourceCommand& command : commands)
        {
            switch (command.type)
            {
                case StrokeCommandType::MoveTo:
                {
                    if (hasContour || (command.firstPoint >= points.Size()))
                    {
                        return false;
                    }

                    outPolygon.PushBack({ points[command.firstPoint].x, points[command.firstPoint].y });
                    hasContour = true;
                    break;
                }

                case StrokeCommandType::LineTo:
                {
                    if (!hasContour || (command.firstPoint >= points.Size()))
                    {
                        return false;
                    }

                    outPolygon.PushBack({ points[command.firstPoint].x, points[command.firstPoint].y });
                    break;
                }

                case StrokeCommandType::Close:
                    if (!hasContour)
                    {
                        return false;
                    }

                    closed = true;
                    break;

                default:
                    return false;
            }
        }

        if (!closed || (outPolygon.Size() < 3u))
        {
            return false;
        }

        const curve_compile::Point2& first = outPolygon[0];
        const curve_compile::Point2& last = outPolygon.Back();
        if ((Abs(first.x - last.x) <= 1.0e-5f) && (Abs(first.y - last.y) <= 1.0e-5f))
        {
            outPolygon.Resize(outPolygon.Size() - 1u);
        }

        return outPolygon.Size() >= 3u;
    }

    inline bool TryBuildClosedLineOnlyFillCurves(
        Vector<curve_compile::CurveData>& outCurves,
        Span<const StrokeSourcePoint> points,
        Span<const StrokeSourceCommand> commands,
        float flatteningTolerance)
    {
        Vector<curve_compile::Point2> polygon{};
        if (!TryExtractSingleClosedLineContour(polygon, points, commands))
        {
            return false;
        }

        BuildLineOnlyCurves(
            outCurves,
            Span<const curve_compile::Point2>(polygon.Data(), polygon.Size()),
            true,
            flatteningTolerance);
        return !outCurves.IsEmpty();
    }
}
