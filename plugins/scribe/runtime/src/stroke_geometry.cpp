// Copyright Chad Engler

#include "stroke_geometry.h"

#include "he/scribe/stroke_outline.h"

#include "he/core/math.h"

#include <algorithm>

namespace he::scribe
{
    namespace
    {
        constexpr uint32_t CurveTextureWidth = 4096;
        constexpr uint32_t MaxBandCount = 8;
        constexpr uint32_t MaxCurveSubdivisionDepth = 8;
        constexpr float DegenerateLineLengthSq = 1.0e-6f;
        constexpr float DegenerateCurveExtent = 1.0e-4f;
        constexpr float Pi = 3.14159265358979323846f;
        constexpr float TwoPi = Pi * 2.0f;

        struct Point2
        {
            float x{ 0.0f };
            float y{ 0.0f };
        };

        struct CurveData
        {
            Point2 p1{};
            Point2 p2{};
            Point2 p3{};
            float minX{ 0.0f };
            float minY{ 0.0f };
            float maxX{ 0.0f };
            float maxY{ 0.0f };
            uint32_t curveTexelIndex{ 0 };
        };

        struct CurveRef
        {
            uint16_t x{ 0 };
            uint16_t y{ 0 };
            float sortKey{ 0.0f };
        };

        struct StrokePath
        {
            Vector<Point2> points{};
            bool closed{ false };
        };

        Point2 operator+(const Point2& a, const Point2& b)
        {
            return { a.x + b.x, a.y + b.y };
        }

        Point2 operator-(const Point2& a, const Point2& b)
        {
            return { a.x - b.x, a.y - b.y };
        }

        Point2 operator*(const Point2& point, float scalar)
        {
            return { point.x * scalar, point.y * scalar };
        }

        float Dot(const Point2& a, const Point2& b)
        {
            return (a.x * b.x) + (a.y * b.y);
        }

        float Cross(const Point2& a, const Point2& b)
        {
            return (a.x * b.y) - (a.y * b.x);
        }

        float LengthSq(const Point2& value)
        {
            return Dot(value, value);
        }

        Point2 LerpPoint(const Point2& a, const Point2& b, float t)
        {
            return {
                Lerp(a.x, b.x, t),
                Lerp(a.y, b.y, t)
            };
        }

        Point2 EvaluateQuadratic(const Point2& p0, const Point2& p1, const Point2& p2, float t)
        {
            const float mt = 1.0f - t;
            const float w0 = mt * mt;
            const float w1 = 2.0f * mt * t;
            const float w2 = t * t;
            return {
                (p0.x * w0) + (p1.x * w1) + (p2.x * w2),
                (p0.y * w0) + (p1.y * w1) + (p2.y * w2)
            };
        }

        Point2 EvaluateCubic(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3, float t)
        {
            const float mt = 1.0f - t;
            const float w0 = mt * mt * mt;
            const float w1 = 3.0f * mt * mt * t;
            const float w2 = 3.0f * mt * t * t;
            const float w3 = t * t * t;
            return {
                (p0.x * w0) + (p1.x * w1) + (p2.x * w2) + (p3.x * w3),
                (p0.y * w0) + (p1.y * w1) + (p2.y * w2) + (p3.y * w3)
            };
        }

        float SignedArea(Span<const Point2> points)
        {
            if (points.Size() < 3)
            {
                return 0.0f;
            }

            float area = 0.0f;
            for (uint32_t pointIndex = 0; pointIndex < points.Size(); ++pointIndex)
            {
                const Point2& a = points[pointIndex];
                const Point2& b = points[(pointIndex + 1) % points.Size()];
                area += (a.x * b.y) - (b.x * a.y);
            }

            return area * 0.5f;
        }

        void ReversePoints(Vector<Point2>& points)
        {
            for (uint32_t i = 0, j = points.Size() > 0 ? points.Size() - 1 : 0; i < j; ++i, --j)
            {
                const Point2 tmp = points[i];
                points[i] = points[j];
                points[j] = tmp;
            }
        }

        float DistanceToLineSq(const Point2& point, const Point2& a, const Point2& b)
        {
            const Point2 ab = b - a;
            const float lenSq = LengthSq(ab);
            if (lenSq <= 1.0e-8f)
            {
                return LengthSq(point - a);
            }

            const float area = Cross(point - a, ab);
            return (area * area) / lenSq;
        }

        Point2 ReduceCubicToQuadraticControl(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3)
        {
            const Point2 startControl{
                ((3.0f * p1.x) - p0.x) * 0.5f,
                ((3.0f * p1.y) - p0.y) * 0.5f
            };
            const Point2 endControl{
                ((3.0f * p2.x) - p3.x) * 0.5f,
                ((3.0f * p2.y) - p3.y) * 0.5f
            };
            return LerpPoint(startControl, endControl, 0.5f);
        }

        float ComputeCubicQuadraticApproximationErrorSq(
            const Point2& p0,
            const Point2& p1,
            const Point2& p2,
            const Point2& p3,
            const Point2& quadraticControl)
        {
            constexpr float SampleTs[] = { 0.25f, 0.5f, 0.75f };

            float maxErrorSq = 0.0f;
            for (float t : SampleTs)
            {
                const Point2 cubicPoint = EvaluateCubic(p0, p1, p2, p3, t);
                const Point2 quadraticPoint = EvaluateQuadratic(p0, quadraticControl, p3, t);
                const float dx = cubicPoint.x - quadraticPoint.x;
                const float dy = cubicPoint.y - quadraticPoint.y;
                maxErrorSq = Max(maxErrorSq, (dx * dx) + (dy * dy));
            }

            return maxErrorSq;
        }

        float ComputeMinimalHalfFloatOffset(float value) noexcept
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

        bool TryComputeStableLineQuadraticControlPoint(
            Point2& outControl,
            const Point2& from,
            const Point2& to) noexcept
        {
            const Point2 delta = to - from;
            const float lenSq = LengthSq(delta);
            if (lenSq <= DegenerateLineLengthSq)
            {
                return false;
            }

            const float invLen = 1.0f / Sqrt(lenSq);
            const float len = lenSq * invLen;
            const Point2 mid = LerpPoint(from, to, 0.5f);
            const float tangentOffset = Min(0.5f, len * 0.25f);
            const Point2 tangent = delta * invLen;
            constexpr float AxisEpsilon = 1.0e-6f;

            outControl.x = mid.x + (tangent.x * tangentOffset);
            outControl.y = mid.y + (tangent.y * tangentOffset);

            if (Abs(delta.x) <= AxisEpsilon)
            {
                outControl.x += ComputeMinimalHalfFloatOffset(mid.x);
            }
            else if (Abs(delta.y) <= AxisEpsilon)
            {
                outControl.y += ComputeMinimalHalfFloatOffset(mid.y);
            }

            return true;
        }

        void AppendCurveTexels(Vector<PackedCurveTexel>& out, const CurveData& curve)
        {
            out.PushBack(PackCurveTexel(curve.p1.x, curve.p1.y, curve.p2.x, curve.p2.y));
            out.PushBack(PackCurveTexel(curve.p3.x, curve.p3.y, 0.0f, 0.0f));
        }

        void AppendLineCurve(Vector<CurveData>& out, const Point2& from, const Point2& to)
        {
            Point2 control{};
            if (!TryComputeStableLineQuadraticControlPoint(control, from, to))
            {
                return;
            }

            const float minX = Min(from.x, to.x);
            const float minY = Min(from.y, to.y);
            const float maxX = Max(from.x, to.x);
            const float maxY = Max(from.y, to.y);
            if (((maxX - minX) <= DegenerateCurveExtent) && ((maxY - minY) <= DegenerateCurveExtent))
            {
                return;
            }

            CurveData& curve = out.EmplaceBack();
            curve.p1 = from;
            curve.p2 = control;
            curve.p3 = to;
            curve.minX = minX;
            curve.minY = minY;
            curve.maxX = maxX;
            curve.maxY = maxY;
        }

        void EmitClosedPolygon(Vector<CurveData>& outCurves, Span<const Point2> polygon)
        {
            if (polygon.Size() < 3)
            {
                return;
            }

            Vector<Point2> points{};
            points.Reserve(polygon.Size());
            for (uint32_t pointIndex = 0; pointIndex < polygon.Size(); ++pointIndex)
            {
                points.PushBack(polygon[pointIndex]);
            }

            if (SignedArea(points) < 0.0f)
            {
                ReversePoints(points);
            }

            for (uint32_t pointIndex = 0; pointIndex < points.Size(); ++pointIndex)
            {
                const Point2& from = points[pointIndex];
                const Point2& to = points[(pointIndex + 1) % points.Size()];
                AppendLineCurve(outCurves, from, to);
            }
        }

        float NormalizeAngle(float angle)
        {
            while (angle <= -Pi)
            {
                angle += TwoPi;
            }

            while (angle > Pi)
            {
                angle -= TwoPi;
            }

            return angle;
        }

        bool AngleOnSweepCCW(float startAngle, float endAngle, float testAngle)
        {
            startAngle = NormalizeAngle(startAngle);
            endAngle = NormalizeAngle(endAngle);
            testAngle = NormalizeAngle(testAngle);

            if (endAngle < startAngle)
            {
                endAngle += TwoPi;
            }

            if (testAngle < startAngle)
            {
                testAngle += TwoPi;
            }

            return (testAngle >= startAngle) && (testAngle <= endAngle);
        }

        void BuildArcPolygon(
            Vector<Point2>& outPolygon,
            const Point2& center,
            const Point2& startPoint,
            const Point2& endPoint,
            float radius,
            bool ccw)
        {
            outPolygon.Clear();
            outPolygon.PushBack(center);
            outPolygon.PushBack(startPoint);

            float startAngle = Atan2(startPoint.y - center.y, startPoint.x - center.x);
            float endAngle = Atan2(endPoint.y - center.y, endPoint.x - center.x);
            if (ccw)
            {
                while (endAngle <= startAngle)
                {
                    endAngle += TwoPi;
                }
            }
            else
            {
                while (endAngle >= startAngle)
                {
                    endAngle -= TwoPi;
                }
            }

            const float sweep = endAngle - startAngle;
            const float tolerance = Max(radius * 0.05f, 0.25f);
            float maxStep = Pi * 0.25f;
            if (radius > tolerance)
            {
                maxStep = Min(maxStep, 2.0f * Acos(Max(1.0f - (tolerance / radius), -1.0f)));
            }

            const uint32_t stepCount = Max(static_cast<uint32_t>(Ceil(Abs(sweep) / Max(maxStep, 0.05f))), 1u);
            for (uint32_t stepIndex = 1; stepIndex < stepCount; ++stepIndex)
            {
                const float t = static_cast<float>(stepIndex) / static_cast<float>(stepCount);
                const float angle = startAngle + (sweep * t);
                outPolygon.PushBack({
                    center.x + (Cos(angle) * radius),
                    center.y + (Sin(angle) * radius)
                });
            }

            outPolygon.PushBack(endPoint);
        }

        bool ComputeLineIntersection(
            Point2& out,
            const Point2& pointA,
            const Point2& dirA,
            const Point2& pointB,
            const Point2& dirB)
        {
            const float denom = Cross(dirA, dirB);
            if (Abs(denom) <= 1.0e-5f)
            {
                return false;
            }

            const float t = Cross(pointB - pointA, dirB) / denom;
            out = pointA + (dirA * t);
            return true;
        }

        void AppendPointUnique(Vector<Point2>& out, const Point2& point)
        {
            if (!out.IsEmpty())
            {
                const Point2 delta = point - out.Back();
                if (LengthSq(delta) <= 1.0e-6f)
                {
                    return;
                }
            }

            out.PushBack(point);
        }

        void FinalizeStrokePath(Vector<StrokePath>& outPaths, StrokePath& current, bool closed)
        {
            if (current.points.Size() < 2)
            {
                current.points.Clear();
                current.closed = false;
                return;
            }

            if (closed && (current.points.Size() > 2))
            {
                const Point2 delta = current.points.Back() - current.points.Front();
                if (LengthSq(delta) <= 1.0e-6f)
                {
                    current.points.PopBack();
                }
            }

            current.closed = closed;
            outPaths.PushBack(Move(current));
            current = {};
        }

        void FlattenQuadratic(
            Vector<Point2>& out,
            const Point2& p0,
            const Point2& p1,
            const Point2& p2,
            float toleranceSq,
            uint32_t depth)
        {
            if ((depth >= MaxCurveSubdivisionDepth) || (DistanceToLineSq(p1, p0, p2) <= toleranceSq))
            {
                AppendPointUnique(out, p2);
                return;
            }

            const Point2 p01 = LerpPoint(p0, p1, 0.5f);
            const Point2 p12 = LerpPoint(p1, p2, 0.5f);
            const Point2 p012 = LerpPoint(p01, p12, 0.5f);

            FlattenQuadratic(out, p0, p01, p012, toleranceSq, depth + 1);
            FlattenQuadratic(out, p012, p12, p2, toleranceSq, depth + 1);
        }

        void FlattenCubic(
            Vector<Point2>& out,
            const Point2& p0,
            const Point2& p1,
            const Point2& p2,
            const Point2& p3,
            float toleranceSq,
            uint32_t depth)
        {
            const float d1 = DistanceToLineSq(p1, p0, p3);
            const float d2 = DistanceToLineSq(p2, p0, p3);
            if ((depth >= MaxCurveSubdivisionDepth) || (Max(d1, d2) <= toleranceSq))
            {
                AppendPointUnique(out, p3);
                return;
            }

            const Point2 p01 = LerpPoint(p0, p1, 0.5f);
            const Point2 p12 = LerpPoint(p1, p2, 0.5f);
            const Point2 p23 = LerpPoint(p2, p3, 0.5f);
            const Point2 p012 = LerpPoint(p01, p12, 0.5f);
            const Point2 p123 = LerpPoint(p12, p23, 0.5f);
            const Point2 p0123 = LerpPoint(p012, p123, 0.5f);

            FlattenCubic(out, p0, p01, p012, p0123, toleranceSq, depth + 1);
            FlattenCubic(out, p0123, p123, p23, p3, toleranceSq, depth + 1);
        }

        bool DecodeStrokePaths(
            Vector<StrokePath>& outPaths,
            float pointScale,
            schema::List<StrokePoint>::Reader points,
            schema::List<StrokeCommand>::Reader commands,
            uint32_t firstCommand,
            uint32_t commandCount,
            float flattenTolerance)
        {
            outPaths.Clear();
            if ((commandCount == 0) || ((firstCommand + commandCount) > commands.Size()))
            {
                return false;
            }

            const float toleranceSq = flattenTolerance * flattenTolerance;
            StrokePath currentPath{};

            auto readPoint = [&](uint32_t pointIndex, Point2& outPoint) -> bool
            {
                if (pointIndex >= points.Size())
                {
                    return false;
                }

                const StrokePoint::Reader point = points[pointIndex];
                outPoint.x = static_cast<float>(point.GetX()) * pointScale;
                outPoint.y = static_cast<float>(point.GetY()) * pointScale;
                return true;
            };

            Point2 currentPoint{};
            bool hasCurrent = false;

            for (uint32_t commandOffset = 0; commandOffset < commandCount; ++commandOffset)
            {
                const StrokeCommand::Reader command = commands[firstCommand + commandOffset];
                const uint32_t pointIndex = command.GetFirstPoint();
                switch (command.GetType())
                {
                    case StrokeCommandType::MoveTo:
                    {
                        if (!currentPath.points.IsEmpty())
                        {
                            FinalizeStrokePath(outPaths, currentPath, false);
                        }

                        Point2 point{};
                        if (!readPoint(pointIndex, point))
                        {
                            return false;
                        }

                        AppendPointUnique(currentPath.points, point);
                        currentPoint = point;
                        hasCurrent = true;
                        break;
                    }

                    case StrokeCommandType::LineTo:
                    {
                        if (!hasCurrent)
                        {
                            return false;
                        }

                        Point2 point{};
                        if (!readPoint(pointIndex, point))
                        {
                            return false;
                        }

                        AppendPointUnique(currentPath.points, point);
                        currentPoint = point;
                        break;
                    }

                    case StrokeCommandType::QuadraticTo:
                    {
                        if (!hasCurrent)
                        {
                            return false;
                        }

                        Point2 control{};
                        Point2 point{};
                        if (!readPoint(pointIndex, control) || !readPoint(pointIndex + 1, point))
                        {
                            return false;
                        }

                        FlattenQuadratic(currentPath.points, currentPoint, control, point, toleranceSq, 0);
                        currentPoint = point;
                        break;
                    }

                    case StrokeCommandType::CubicTo:
                    {
                        if (!hasCurrent)
                        {
                            return false;
                        }

                        Point2 control1{};
                        Point2 control2{};
                        Point2 point{};
                        if (!readPoint(pointIndex, control1)
                            || !readPoint(pointIndex + 1, control2)
                            || !readPoint(pointIndex + 2, point))
                        {
                            return false;
                        }

                        FlattenCubic(currentPath.points, currentPoint, control1, control2, point, toleranceSq, 0);
                        currentPoint = point;
                        break;
                    }

                    case StrokeCommandType::Close:
                        if (!currentPath.points.IsEmpty())
                        {
                            FinalizeStrokePath(outPaths, currentPath, true);
                            hasCurrent = false;
                        }
                        break;

                    default:
                        return false;
                }
            }

            if (!currentPath.points.IsEmpty())
            {
                FinalizeStrokePath(outPaths, currentPath, false);
            }

            return !outPaths.IsEmpty();
        }

        float Length(const Point2& value)
        {
            return Sqrt(LengthSq(value));
        }

        Point2 Normalize(const Point2& value)
        {
            const float lengthSq = LengthSq(value);
            if (lengthSq <= DegenerateLineLengthSq)
            {
                return {};
            }

            const float invLength = 1.0f / Sqrt(lengthSq);
            return value * invLength;
        }

        Point2 LeftNormal(const Point2& direction)
        {
            return { -direction.y, direction.x };
        }

        void EmitRoundCap(
            Vector<CurveData>& outCurves,
            const Point2& center,
            const Point2& startPoint,
            const Point2& endPoint,
            const Point2& exteriorDirection,
            float radius)
        {
            const float startAngle = Atan2(startPoint.y - center.y, startPoint.x - center.x);
            const float endAngle = Atan2(endPoint.y - center.y, endPoint.x - center.x);
            const float exteriorAngle = Atan2(exteriorDirection.y, exteriorDirection.x);
            const bool ccw = AngleOnSweepCCW(startAngle, endAngle, exteriorAngle);

            Vector<Point2> polygon{};
            BuildArcPolygon(polygon, center, startPoint, endPoint, radius, ccw);
            EmitClosedPolygon(outCurves, polygon);
        }

        void EmitJoinPatch(
            Vector<CurveData>& outCurves,
            const Point2& vertex,
            const Point2& dirIn,
            const Point2& dirOut,
            float halfWidth,
            const StrokeStyle& style)
        {
            const float turn = Cross(dirIn, dirOut);
            if (Abs(turn) <= 1.0e-5f)
            {
                return;
            }

            Point2 offsetIn = LeftNormal(dirIn) * halfWidth;
            Point2 offsetOut = LeftNormal(dirOut) * halfWidth;
            if (turn < 0.0f)
            {
                offsetIn = offsetIn * -1.0f;
                offsetOut = offsetOut * -1.0f;
            }

            const Point2 outerStart = vertex + offsetIn;
            const Point2 outerEnd = vertex + offsetOut;
            Vector<Point2> polygon{};

            switch (style.joinStyle)
            {
                case StrokeJoinStyle::Round:
                    BuildArcPolygon(polygon, vertex, outerStart, outerEnd, halfWidth, turn > 0.0f);
                    EmitClosedPolygon(outCurves, polygon);
                    return;

                case StrokeJoinStyle::Miter:
                {
                    Point2 intersection{};
                    if (ComputeLineIntersection(intersection, outerStart, dirIn, outerEnd, dirOut))
                    {
                        const float miterLength = Length(intersection - vertex) / Max(halfWidth, 1.0e-5f);
                        if (miterLength <= Max(style.miterLimit, 1.0f))
                        {
                            polygon.PushBack(vertex);
                            polygon.PushBack(outerStart);
                            polygon.PushBack(intersection);
                            polygon.PushBack(outerEnd);
                            EmitClosedPolygon(outCurves, polygon);
                            return;
                        }
                    }
                    break;
                }

                case StrokeJoinStyle::Bevel:
                default:
                    break;
            }

            polygon.PushBack(vertex);
            polygon.PushBack(outerStart);
            polygon.PushBack(outerEnd);
            EmitClosedPolygon(outCurves, polygon);
        }

        void BuildStrokeCurves(
            Vector<CurveData>& outCurves,
            Span<const StrokePath> paths,
            const StrokeStyle& style)
        {
            outCurves.Clear();
            if (!style.IsVisible())
            {
                return;
            }

            const float halfWidth = style.width * 0.5f;
            for (const StrokePath& path : paths)
            {
                if (path.points.Size() < 2)
                {
                    continue;
                }

                const uint32_t segmentCount = path.closed ? path.points.Size() : (path.points.Size() - 1);
                for (uint32_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
                {
                    const uint32_t nextIndex = (segmentIndex + 1) % path.points.Size();
                    Point2 start = path.points[segmentIndex];
                    Point2 end = path.points[nextIndex];
                    const Point2 direction = Normalize(end - start);
                    if (LengthSq(direction) <= 1.0e-6f)
                    {
                        continue;
                    }

                    if (!path.closed && (style.capStyle == StrokeCapStyle::Square))
                    {
                        if (segmentIndex == 0)
                        {
                            start = start - (direction * halfWidth);
                        }

                        if (segmentIndex == (segmentCount - 1))
                        {
                            end = end + (direction * halfWidth);
                        }
                    }

                    const Point2 normal = LeftNormal(direction) * halfWidth;
                    const Point2 polygon[] =
                    {
                        start + normal,
                        start - normal,
                        end - normal,
                        end + normal,
                    };
                    EmitClosedPolygon(outCurves, polygon);
                }

                const uint32_t joinStart = path.closed ? 0u : 1u;
                const uint32_t joinEnd = path.closed ? path.points.Size() : (path.points.Size() - 1);
                for (uint32_t pointIndex = joinStart; pointIndex < joinEnd; ++pointIndex)
                {
                    const uint32_t prevIndex = pointIndex == 0 ? (path.points.Size() - 1) : (pointIndex - 1);
                    const uint32_t nextIndex = (pointIndex + 1) % path.points.Size();
                    const Point2 dirIn = Normalize(path.points[pointIndex] - path.points[prevIndex]);
                    const Point2 dirOut = Normalize(path.points[nextIndex] - path.points[pointIndex]);
                    if ((LengthSq(dirIn) <= 1.0e-6f) || (LengthSq(dirOut) <= 1.0e-6f))
                    {
                        continue;
                    }

                    EmitJoinPatch(outCurves, path.points[pointIndex], dirIn, dirOut, halfWidth, style);
                }

                if (!path.closed && (style.capStyle == StrokeCapStyle::Round))
                {
                    const Point2 startDir = Normalize(path.points[1] - path.points[0]);
                    const Point2 startNormal = LeftNormal(startDir) * halfWidth;
                    EmitRoundCap(
                        outCurves,
                        path.points[0],
                        path.points[0] - startNormal,
                        path.points[0] + startNormal,
                        startDir * -1.0f,
                        halfWidth);

                    const uint32_t lastPointIndex = path.points.Size() - 1;
                    const Point2 endDir = Normalize(path.points[lastPointIndex] - path.points[lastPointIndex - 1]);
                    const Point2 endNormal = LeftNormal(endDir) * halfWidth;
                    EmitRoundCap(
                        outCurves,
                        path.points[lastPointIndex],
                        path.points[lastPointIndex] + endNormal,
                        path.points[lastPointIndex] - endNormal,
                        endDir,
                        halfWidth);
                }
            }
        }

        void GetBandRange(
            int32_t& outStart,
            int32_t& outEnd,
            float curveMin,
            float curveMax,
            float boundsMin,
            float boundsMax,
            uint32_t bandCount,
            float epsilon)
        {
            const float boundsSpan = boundsMax - boundsMin;
            if ((bandCount <= 1) || (boundsSpan <= epsilon))
            {
                outStart = 0;
                outEnd = 0;
                return;
            }

            const float bandScale = static_cast<float>(bandCount) / boundsSpan;
            const float startBand = Floor(((curveMin - epsilon) - boundsMin) * bandScale);
            const float endBand = Floor(((curveMax + epsilon) - boundsMin) * bandScale);
            outStart = Clamp(static_cast<int32_t>(startBand), 0, static_cast<int32_t>(bandCount - 1));
            outEnd = Clamp(static_cast<int32_t>(endBand), 0, static_cast<int32_t>(bandCount - 1));
        }

        uint32_t ChooseBandCount(
            const Vector<CurveData>& curves,
            bool horizontalBands,
            float boundsMin,
            float boundsMax,
            float epsilon)
        {
            const float span = boundsMax - boundsMin;
            if (curves.IsEmpty() || (span <= epsilon))
            {
                return 1;
            }

            uint32_t bestBandCount = 1;
            uint32_t bestMaxOccupancy = curves.Size();
            Vector<uint32_t> counts{};

            for (uint32_t bandCount = 1; bandCount <= MaxBandCount; ++bandCount)
            {
                counts.Resize(bandCount, DefaultInit);
                for (uint32_t i = 0; i < counts.Size(); ++i)
                {
                    counts[i] = 0;
                }

                for (const CurveData& curve : curves)
                {
                    int32_t startBand = 0;
                    int32_t endBand = 0;
                    GetBandRange(
                        startBand,
                        endBand,
                        horizontalBands ? curve.minY : curve.minX,
                        horizontalBands ? curve.maxY : curve.maxX,
                        boundsMin,
                        boundsMax,
                        bandCount,
                        epsilon);

                    for (int32_t bandIndex = startBand; bandIndex <= endBand; ++bandIndex)
                    {
                        ++counts[static_cast<uint32_t>(bandIndex)];
                    }
                }

                uint32_t maxOccupancy = 0;
                for (uint32_t count : counts)
                {
                    maxOccupancy = Max(maxOccupancy, count);
                }

                if ((maxOccupancy < bestMaxOccupancy)
                    || ((maxOccupancy == bestMaxOccupancy) && (bandCount < bestBandCount)))
                {
                    bestBandCount = bandCount;
                    bestMaxOccupancy = maxOccupancy;
                }
            }

            return bestBandCount;
        }

        void BuildBandRefs(
            Vector<Vector<CurveRef>>& outBands,
            const Vector<CurveData>& curves,
            bool horizontalBands,
            float boundsMin,
            float boundsMax,
            uint32_t bandCount,
            float epsilon)
        {
            outBands.Clear();
            outBands.Resize(bandCount);

            for (const CurveData& curve : curves)
            {
                int32_t startBand = 0;
                int32_t endBand = 0;
                GetBandRange(
                    startBand,
                    endBand,
                    horizontalBands ? curve.minY : curve.minX,
                    horizontalBands ? curve.maxY : curve.maxX,
                    boundsMin,
                    boundsMax,
                    bandCount,
                    epsilon);

                CurveRef ref{};
                ref.x = static_cast<uint16_t>(curve.curveTexelIndex & (CurveTextureWidth - 1));
                ref.y = static_cast<uint16_t>(curve.curveTexelIndex / CurveTextureWidth);
                ref.sortKey = horizontalBands ? curve.maxX : curve.maxY;

                for (int32_t bandIndex = startBand; bandIndex <= endBand; ++bandIndex)
                {
                    outBands[static_cast<uint32_t>(bandIndex)].PushBack(ref);
                }
            }

            for (Vector<CurveRef>& band : outBands)
            {
                if (band.Size() > 1)
                {
                    std::sort(band.Data(), band.Data() + band.Size(), [](const CurveRef& a, const CurveRef& b)
                    {
                        return a.sortKey > b.sortKey;
                    });
                }
            }
        }

        void AppendPackedBands(
            Vector<PackedBandTexel>& outBandTexels,
            uint32_t glyphBandStart,
            const Vector<Vector<CurveRef>>& horizontalBands,
            const Vector<Vector<CurveRef>>& verticalBands)
        {
            struct ExistingBandPayload
            {
                uint32_t offset{ 0 };
                uint32_t size{ 0 };
            };

            const uint32_t headerCount = horizontalBands.Size() + verticalBands.Size();
            const uint32_t payloadStart = glyphBandStart + headerCount;
            outBandTexels.Resize(payloadStart);

            uint32_t currentOffset = headerCount;
            Vector<ExistingBandPayload> existingBands{};
            auto appendBand = [&](uint32_t headerIndex, const Vector<CurveRef>& band)
            {
                PackedBandTexel header{};
                header.x = static_cast<uint16_t>(band.Size());

                if (!band.IsEmpty())
                {
                    constexpr uint32_t InvalidOffset = 0xFFFFFFFFu;
                    uint32_t reuseOffset = InvalidOffset;
                    for (const ExistingBandPayload& existing : existingBands)
                    {
                        if (existing.size != band.Size())
                        {
                            continue;
                        }

                        bool matches = true;
                        for (uint32_t curveIndex = 0; curveIndex < band.Size(); ++curveIndex)
                        {
                            const PackedBandTexel& existingTexel = outBandTexels[payloadStart + existing.offset + curveIndex];
                            if ((existingTexel.x != band[curveIndex].x) || (existingTexel.y != band[curveIndex].y))
                            {
                                matches = false;
                                break;
                            }
                        }

                        if (matches)
                        {
                            reuseOffset = existing.offset;
                            break;
                        }
                    }

                    if (reuseOffset != InvalidOffset)
                    {
                        header.y = static_cast<uint16_t>(headerCount + reuseOffset);
                    }
                    else
                    {
                        header.y = static_cast<uint16_t>(currentOffset);
                        ExistingBandPayload& existing = existingBands.EmplaceBack();
                        existing.offset = currentOffset - headerCount;
                        existing.size = band.Size();

                        for (const CurveRef& ref : band)
                        {
                            PackedBandTexel& texel = outBandTexels.EmplaceBack();
                            texel.x = ref.x;
                            texel.y = ref.y;
                        }

                        currentOffset += band.Size();
                    }
                }
                else
                {
                    header.y = static_cast<uint16_t>(currentOffset);
                }

                outBandTexels[glyphBandStart + headerIndex] = header;
            };

            for (uint32_t bandIndex = 0; bandIndex < horizontalBands.Size(); ++bandIndex)
            {
                appendBand(bandIndex, horizontalBands[bandIndex]);
            }

            for (uint32_t bandIndex = 0; bandIndex < verticalBands.Size(); ++bandIndex)
            {
                appendBand(horizontalBands.Size() + bandIndex, verticalBands[bandIndex]);
            }
        }

        float ComputeBandScale(float minBound, float maxBound, uint32_t bandCount)
        {
            if (bandCount <= 1)
            {
                return 0.0f;
            }

            const float span = maxBound - minBound;
            if (span <= 0.0f)
            {
                return 0.0f;
            }

            return static_cast<float>(bandCount) / span;
        }

        void PadCurveTexture(Vector<PackedCurveTexel>& texels, uint32_t width, uint32_t& outHeight)
        {
            const uint32_t texelCount = Max(texels.Size(), 1u);
            outHeight = (texelCount + (width - 1)) / width;
            texels.Resize(width * outHeight);
        }

        void PadBandTexture(Vector<PackedBandTexel>& texels, uint32_t width, uint32_t& outHeight)
        {
            const uint32_t texelCount = Max(texels.Size(), width);
            outHeight = (texelCount + (width - 1)) / width;
            texels.Resize(width * outHeight);
        }
    }

    bool BuildStrokedShapeData(
        StrokedShapeData& out,
        float pointScale,
        schema::List<StrokePoint>::Reader points,
        schema::List<StrokeCommand>::Reader commands,
        uint32_t firstCommand,
        uint32_t commandCount,
        const StrokeStyle& style)
    {
        out = {};
        if (!style.IsVisible())
        {
            return false;
        }

        Vector<CurveData> curves{};
        StrokeOutlineStyle outlineStyle{};
        outlineStyle.width = style.width;
        outlineStyle.join = style.joinStyle == StrokeJoinStyle::Bevel
            ? StrokeJoinKind::Bevel
            : style.joinStyle == StrokeJoinStyle::Round
                ? StrokeJoinKind::Round
                : StrokeJoinKind::Miter;
        outlineStyle.cap = style.capStyle == StrokeCapStyle::Square
            ? StrokeCapKind::Square
            : style.capStyle == StrokeCapStyle::Round
                ? StrokeCapKind::Round
                : StrokeCapKind::Butt;
        outlineStyle.miterLimit = style.miterLimit;

        Vector<StrokeOutlineCurve> outlineCurves{};
        if (!BuildStrokedOutlineCurves(
                outlineCurves,
                pointScale,
                points,
                commands,
                firstCommand,
                commandCount,
                outlineStyle))
        {
            return false;
        }

        auto appendQuadraticCurve = [&](const Point2& p0, const Point2& p1, const Point2& p2, bool isLineLike)
        {
            float minX = 0.0f;
            float minY = 0.0f;
            float maxX = 0.0f;
            float maxY = 0.0f;
            if (isLineLike)
            {
                minX = Min(p0.x, p2.x);
                minY = Min(p0.y, p2.y);
                maxX = Max(p0.x, p2.x);
                maxY = Max(p0.y, p2.y);
            }
            else
            {
                minX = Min(p0.x, p1.x, p2.x);
                minY = Min(p0.y, p1.y, p2.y);
                maxX = Max(p0.x, p1.x, p2.x);
                maxY = Max(p0.y, p1.y, p2.y);
            }

            if (((maxX - minX) <= DegenerateCurveExtent) && ((maxY - minY) <= DegenerateCurveExtent))
            {
                return;
            }

            CurveData& curve = curves.EmplaceBack();
            curve.p1 = p0;
            curve.p2 = p1;
            curve.p3 = p2;
            curve.minX = minX;
            curve.minY = minY;
            curve.maxX = maxX;
            curve.maxY = maxY;
        };

        auto appendOutlineLine = [&](const Point2& from, const Point2& to)
        {
            Point2 control{};
            if (!TryComputeStableLineQuadraticControlPoint(control, from, to))
            {
                return;
            }

            appendQuadraticCurve(from, control, to, true);
        };

        const float cubicToleranceSq = Max(style.width * 0.01f, 0.01f) * Max(style.width * 0.01f, 0.01f);

        auto flattenOutlineCubic = [&](auto&& self, const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3, uint32_t depth) -> void
        {
            const Point2 quadraticControl = ReduceCubicToQuadraticControl(p0, p1, p2, p3);
            const float errorSq = ComputeCubicQuadraticApproximationErrorSq(p0, p1, p2, p3, quadraticControl);
            if ((depth >= MaxCurveSubdivisionDepth) || (errorSq <= cubicToleranceSq))
            {
                appendQuadraticCurve(p0, quadraticControl, p3, false);
                return;
            }

            const Point2 p01 = LerpPoint(p0, p1, 0.5f);
            const Point2 p12 = LerpPoint(p1, p2, 0.5f);
            const Point2 p23 = LerpPoint(p2, p3, 0.5f);
            const Point2 p012 = LerpPoint(p01, p12, 0.5f);
            const Point2 p123 = LerpPoint(p12, p23, 0.5f);
            const Point2 p0123 = LerpPoint(p012, p123, 0.5f);

            self(self, p0, p01, p012, p0123, depth + 1);
            self(self, p0123, p123, p23, p3, depth + 1);
        };

        for (const StrokeOutlineCurve& outlineCurve : outlineCurves)
        {
            const Point2 p0{ outlineCurve.x0, outlineCurve.y0 };
            if (outlineCurve.kind == StrokeOutlineCurveKind::Line)
            {
                appendOutlineLine(p0, { outlineCurve.x1, outlineCurve.y1 });
            }
            else if (outlineCurve.kind == StrokeOutlineCurveKind::Quadratic)
            {
                appendQuadraticCurve(p0, { outlineCurve.x1, outlineCurve.y1 }, { outlineCurve.x2, outlineCurve.y2 }, false);
            }
            else
            {
                flattenOutlineCubic(
                    flattenOutlineCubic,
                    p0,
                    { outlineCurve.x1, outlineCurve.y1 },
                    { outlineCurve.x2, outlineCurve.y2 },
                    { outlineCurve.x3, outlineCurve.y3 },
                    0);
            }
        }

        if (curves.IsEmpty())
        {
            return false;
        }

        out.fillRule = FillRule::NonZero;
        out.boundsMinX = curves[0].minX;
        out.boundsMinY = curves[0].minY;
        out.boundsMaxX = curves[0].maxX;
        out.boundsMaxY = curves[0].maxY;
        for (CurveData& curve : curves)
        {
            curve.curveTexelIndex = out.curveTexels.Size();
            AppendCurveTexels(out.curveTexels, curve);
            out.boundsMinX = Min(out.boundsMinX, curve.minX);
            out.boundsMinY = Min(out.boundsMinY, curve.minY);
            out.boundsMaxX = Max(out.boundsMaxX, curve.maxX);
            out.boundsMaxY = Max(out.boundsMaxY, curve.maxY);
        }

        const float epsilon = Max(style.width * 0.125f, 0.5f);
        const uint32_t bandCountX = ChooseBandCount(curves, false, out.boundsMinX, out.boundsMaxX, epsilon);
        const uint32_t bandCountY = ChooseBandCount(curves, true, out.boundsMinY, out.boundsMaxY, epsilon);
        out.bandMaxX = bandCountX > 0 ? (bandCountX - 1) : 0;
        out.bandMaxY = bandCountY > 0 ? (bandCountY - 1) : 0;
        out.bandScaleX = ComputeBandScale(out.boundsMinX, out.boundsMaxX, bandCountX);
        out.bandScaleY = ComputeBandScale(out.boundsMinY, out.boundsMaxY, bandCountY);
        out.bandOffsetX = -out.boundsMinX * out.bandScaleX;
        out.bandOffsetY = -out.boundsMinY * out.bandScaleY;

        Vector<Vector<CurveRef>> horizontalBands{};
        Vector<Vector<CurveRef>> verticalBands{};
        BuildBandRefs(horizontalBands, curves, true, out.boundsMinY, out.boundsMaxY, bandCountY, epsilon);
        BuildBandRefs(verticalBands, curves, false, out.boundsMinX, out.boundsMaxX, bandCountX, epsilon);
        AppendPackedBands(out.bandTexels, 0, horizontalBands, verticalBands);

        out.curveTextureWidth = CurveTextureWidth;
        out.bandTextureWidth = ScribeBandTextureWidth;
        PadCurveTexture(out.curveTexels, out.curveTextureWidth, out.curveTextureHeight);
        PadBandTexture(out.bandTexels, out.bandTextureWidth, out.bandTextureHeight);
        return true;
    }
}
