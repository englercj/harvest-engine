// Copyright Chad Engler

#include "image_compile_geometry.h"

#include "curve_compile_utils.h"
#include "stroke_compile_utils.h"

#include "font_import_utils.h"

#include "he/scribe/stroke_outline.h"

#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/math.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/string_view.h"
#include "he/core/utf8.h"
#include "he/core/utils.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <algorithm>

namespace he::scribe::editor
{
    namespace
    {
        constexpr uint32_t CurveTextureWidth = curve_compile::CurveTextureWidth;
        constexpr uint32_t MaxBandCount = curve_compile::MaxBandCount;
        constexpr uint32_t MaxCubicSubdivisionDepth = curve_compile::MaxCubicSubdivisionDepth;
        constexpr float DefaultBandOverlapEpsilon = 1.0f / 1024.0f;
        constexpr float DegenerateLineLengthSq = curve_compile::DegenerateLineLengthSq;
        constexpr float DegenerateCurveExtent = curve_compile::DegenerateCurveExtent;
        using curve_compile::Affine2D;
        using curve_compile::CurveData;
        using curve_compile::CurveRef;
        using curve_compile::Point2;

        struct ParsedShape
        {
            Vector<CurveData> curves{};
            Vector<StrokeSourcePoint> strokePoints{};
            Vector<StrokeSourceCommand> strokeCommands{};
            FillRule fillRule{ FillRule::NonZero };
            Vec4f color{ 0.0f, 0.0f, 0.0f, 1.0f };
            float minX{ 0.0f };
            float minY{ 0.0f };
            float maxX{ 0.0f };
            float maxY{ 0.0f };
        };

        struct ParsedClipPath
        {
            String id{};
            float minX{ 0.0f };
            float minY{ 0.0f };
            float maxX{ 0.0f };
            float maxY{ 0.0f };
        };

        struct ParsedDefinition
        {
            String id{};
            ParsedShape shape{};
        };

        struct ParsedImage
        {
            Vector<ParsedShape> shapes{};
            Vector<CompiledVectorImageLayerEntry> layers{};
            Vector<CompiledVectorImageFontFaceEntry> fontFaces{};
            Vector<CompiledVectorImageTextRunEntry> textRuns{};
            float viewBoxMinX{ 0.0f };
            float viewBoxMinY{ 0.0f };
            float viewBoxWidth{ 0.0f };
            float viewBoxHeight{ 0.0f };
            float boundsMinX{ 0.0f };
            float boundsMinY{ 0.0f };
            float boundsMaxX{ 0.0f };
            float boundsMaxY{ 0.0f };
            bool hasViewBox{ false };
        };

        struct StyleState
        {
            Vec4f fill{ 0.0f, 0.0f, 0.0f, 1.0f };
            Vec4f stroke{ 0.0f, 0.0f, 0.0f, 0.0f };
            FillRule fillRule{ FillRule::NonZero };
            bool fillNone{ false };
            bool strokeNone{ true };
            float strokeWidth{ 1.0f };
            StrokeJoinKind strokeJoin{ StrokeJoinKind::Miter };
            StrokeCapKind strokeCap{ StrokeCapKind::Butt };
            float strokeMiterLimit{ 4.0f };
            String strokeDashArray{};
        };

        struct ParseState
        {
            Affine2D transform{};
            StyleState style{};
            bool inDefinitions{ false };
            bool inClipPath{ false };
            bool suppressOutput{ false };
            StringView activeClipPathId{};
            StringView clipPathRef{};
        };

        struct Attribute
        {
            StringView name{};
            StringView value{};
        };

        bool ParseDashArray(Vector<float>& out, StringView text);

        bool ParseDashArrayScaled(Vector<float>& out, StringView text, float scale)
        {
            if (!ParseDashArray(out, text))
            {
                return false;
            }

            if (scale != 1.0f)
            {
                for (float& value : out)
                {
                    value *= scale;
                }
            }

            return true;
        }

        float DegreesToRadians(float degrees)
        {
            return degrees * 0.017453292519943295769f;
        }

        Point2 ReflectPoint(const Point2& around, const Point2& control)
        {
            return {
                (around.x * 2.0f) - control.x,
                (around.y * 2.0f) - control.y
            };
        }

        void RecomputeShapeBounds(ParsedShape& shape)
        {
            if (shape.curves.IsEmpty())
            {
                if (shape.strokePoints.IsEmpty())
                {
                    shape.minX = 0.0f;
                    shape.minY = 0.0f;
                    shape.maxX = 0.0f;
                    shape.maxY = 0.0f;
                    return;
                }

                shape.minX = shape.strokePoints[0].x;
                shape.minY = shape.strokePoints[0].y;
                shape.maxX = shape.strokePoints[0].x;
                shape.maxY = shape.strokePoints[0].y;
                for (uint32_t pointIndex = 1; pointIndex < shape.strokePoints.Size(); ++pointIndex)
                {
                    const StrokeSourcePoint& point = shape.strokePoints[pointIndex];
                    shape.minX = Min(shape.minX, point.x);
                    shape.minY = Min(shape.minY, point.y);
                    shape.maxX = Max(shape.maxX, point.x);
                    shape.maxY = Max(shape.maxY, point.y);
                }
                return;
            }

            shape.minX = shape.curves[0].minX;
            shape.minY = shape.curves[0].minY;
            shape.maxX = shape.curves[0].maxX;
            shape.maxY = shape.curves[0].maxY;
            for (uint32_t curveIndex = 1; curveIndex < shape.curves.Size(); ++curveIndex)
            {
                const CurveData& curve = shape.curves[curveIndex];
                shape.minX = Min(shape.minX, curve.minX);
                shape.minY = Min(shape.minY, curve.minY);
                shape.maxX = Max(shape.maxX, curve.maxX);
                shape.maxY = Max(shape.maxY, curve.maxY);
            }
        }

        void TransformCurveBounds(CurveData& curve)
        {
            if (curve.isLineLike)
            {
                curve.minX = Min(curve.p1.x, curve.p3.x);
                curve.minY = Min(curve.p1.y, curve.p3.y);
                curve.maxX = Max(curve.p1.x, curve.p3.x);
                curve.maxY = Max(curve.p1.y, curve.p3.y);
                return;
            }

            curve.minX = Min(curve.p1.x, Min(curve.p2.x, curve.p3.x));
            curve.minY = Min(curve.p1.y, Min(curve.p2.y, curve.p3.y));
            curve.maxX = Max(curve.p1.x, Max(curve.p2.x, curve.p3.x));
            curve.maxY = Max(curve.p1.y, Max(curve.p2.y, curve.p3.y));
        }

        ParsedShape CloneTransformedShape(const ParsedShape& source, const ParseState& state)
        {
            ParsedShape shape = source;
            shape.fillRule = state.style.fillRule;
            shape.color = state.style.fill;
            for (CurveData& curve : shape.curves)
            {
                curve.p1 = TransformPoint(state.transform, curve.p1);
                curve.p2 = TransformPoint(state.transform, curve.p2);
                curve.p3 = TransformPoint(state.transform, curve.p3);
                TransformCurveBounds(curve);
            }

            for (StrokeSourcePoint& point : shape.strokePoints)
            {
                const Point2 transformed = TransformPoint(state.transform, { point.x, point.y });
                point.x = transformed.x;
                point.y = transformed.y;
            }

            RecomputeShapeBounds(shape);
            return shape;
        }

        const Attribute* FindAttribute(Span<const Attribute> attrs, StringView name)
        {
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI(name))
                {
                    return &attr;
                }
            }

            return nullptr;
        }

        Affine2D ComposeAffine(const Affine2D& outer, const Affine2D& inner)
        {
            Affine2D result{};
            result.m00 = (outer.m00 * inner.m00) + (outer.m01 * inner.m10);
            result.m01 = (outer.m00 * inner.m01) + (outer.m01 * inner.m11);
            result.m10 = (outer.m10 * inner.m00) + (outer.m11 * inner.m10);
            result.m11 = (outer.m10 * inner.m01) + (outer.m11 * inner.m11);
            result.tx = (outer.m00 * inner.tx) + (outer.m01 * inner.ty) + outer.tx;
            result.ty = (outer.m10 * inner.tx) + (outer.m11 * inner.ty) + outer.ty;
            return result;
        }

        Affine2D MakeTranslateAffine(float tx, float ty)
        {
            Affine2D result{};
            result.tx = tx;
            result.ty = ty;
            return result;
        }

        Affine2D MakeScaleAffine(float sx, float sy)
        {
            Affine2D result{};
            result.m00 = sx;
            result.m11 = sy;
            return result;
        }

        Affine2D MakeRotateAffine(float degrees)
        {
            const float radians = DegreesToRadians(degrees);
            float s = 0.0f;
            float c = 1.0f;
            SinCos(radians, s, c);

            Affine2D result{};
            result.m00 = c;
            result.m01 = -s;
            result.m10 = s;
            result.m11 = c;
            return result;
        }

        Affine2D MakeRotateAffine(float degrees, float cx, float cy)
        {
            return ComposeAffine(
                MakeTranslateAffine(cx, cy),
                ComposeAffine(
                    MakeRotateAffine(degrees),
                    MakeTranslateAffine(-cx, -cy)));
        }

        Affine2D MakeSkewXAffine(float degrees)
        {
            Affine2D result{};
            result.m01 = Tan(DegreesToRadians(degrees));
            return result;
        }

        Affine2D MakeSkewYAffine(float degrees)
        {
            Affine2D result{};
            result.m10 = Tan(DegreesToRadians(degrees));
            return result;
        }

        float ComputeAffineStrokeMetricScale(const Affine2D& transform)
        {
            const float scaleX = Sqrt((transform.m00 * transform.m00) + (transform.m10 * transform.m10));
            const float scaleY = Sqrt((transform.m01 * transform.m01) + (transform.m11 * transform.m11));
            if ((scaleX <= 1.0e-6f) && (scaleY <= 1.0e-6f))
            {
                return 1.0f;
            }

            if (scaleX <= 1.0e-6f)
            {
                return scaleY;
            }

            if (scaleY <= 1.0e-6f)
            {
                return scaleX;
            }

            return 0.5f * (scaleX + scaleY);
        }

        StyleState BuildEffectiveStrokeStyle(const StyleState& source, const Affine2D& transform)
        {
            StyleState result = source;
            result.strokeWidth *= ComputeAffineStrokeMetricScale(transform);
            return result;
        }

        [[maybe_unused]] float SvgDistanceToLineSq(const Point2& point, const Point2& a, const Point2& b)
        {
            return curve_compile::DistanceToLineSq(point, a, b);
        }

        void SvgAppendCurveTexels(Vector<PackedCurveTexel>& out, const CurveData& curve)
        {
            curve_compile::AppendCurveTexels(out, curve);
        }

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

        float SvgDot(const Point2& a, const Point2& b)
        {
            return (a.x * b.x) + (a.y * b.y);
        }

        float SvgCross(const Point2& a, const Point2& b)
        {
            return (a.x * b.y) - (a.y * b.x);
        }

        float SvgLengthSq(const Point2& value)
        {
            return SvgDot(value, value);
        }

        Point2 SvgLerpPoint(const Point2& a, const Point2& b, float t)
        {
            return {
                Lerp(a.x, b.x, t),
                Lerp(a.y, b.y, t)
            };
        }

        float SvgSignedArea(Span<const Point2> points)
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

        void SvgReversePoints(Vector<Point2>& points)
        {
            for (uint32_t i = 0, j = points.Size() > 0 ? points.Size() - 1 : 0; i < j; ++i, --j)
            {
                const Point2 tmp = points[i];
                points[i] = points[j];
                points[j] = tmp;
            }
        }

        void SvgAppendLineCurve(Vector<CurveData>& out, const Point2& from, const Point2& to)
        {
            LineCurvePoint control{};
            if (!TryComputeStableLineQuadraticControlPoint(
                    control,
                    { from.x, from.y },
                    { to.x, to.y },
                    DegenerateLineLengthSq))
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
            curve.p2 = { control.x, control.y };
            curve.p3 = to;
            curve.minX = minX;
            curve.minY = minY;
            curve.maxX = maxX;
            curve.maxY = maxY;
            curve.isLineLike = true;
        }

        void SvgEmitClosedPolygon(Vector<CurveData>& outCurves, Span<const Point2> polygon)
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

            if (SvgSignedArea(points) < 0.0f)
            {
                SvgReversePoints(points);
            }

            for (uint32_t pointIndex = 0; pointIndex < points.Size(); ++pointIndex)
            {
                const Point2& from = points[pointIndex];
                const Point2& to = points[(pointIndex + 1) % points.Size()];
                SvgAppendLineCurve(outCurves, from, to);
            }
        }

        float SvgNormalizeAngle(float angle)
        {
            constexpr float Pi = 3.14159265358979323846f;
            constexpr float TwoPi = Pi * 2.0f;
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

        bool SvgAngleOnSweepCCW(float startAngle, float endAngle, float testAngle)
        {
            constexpr float TwoPi = 6.28318530717958647692f;

            startAngle = SvgNormalizeAngle(startAngle);
            endAngle = SvgNormalizeAngle(endAngle);
            testAngle = SvgNormalizeAngle(testAngle);

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

        void SvgBuildArcPolygon(
            Vector<Point2>& outPolygon,
            const Point2& center,
            const Point2& startPoint,
            const Point2& endPoint,
            float radius,
            bool ccw)
        {
            constexpr float Pi = 3.14159265358979323846f;
            constexpr float TwoPi = Pi * 2.0f;

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

        bool SvgComputeLineIntersection(
            Point2& out,
            const Point2& pointA,
            const Point2& dirA,
            const Point2& pointB,
            const Point2& dirB)
        {
            const float denom = SvgCross(dirA, dirB);
            if (Abs(denom) <= 1.0e-5f)
            {
                return false;
            }

            const float t = SvgCross(pointB - pointA, dirB) / denom;
            out = pointA + (dirA * t);
            return true;
        }

        void SvgAppendPointUnique(Vector<Point2>& out, const Point2& point)
        {
            if (!out.IsEmpty())
            {
                const Point2 delta = point - out.Back();
                if (SvgLengthSq(delta) <= 1.0e-6f)
                {
                    return;
                }
            }

            out.PushBack(point);
        }

        struct SvgStrokePath
        {
            Vector<Point2> points{};
            bool closed{ false };
        };

        void SvgFinalizeStrokePath(Vector<SvgStrokePath>& outPaths, SvgStrokePath& current, bool closed)
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
                if (SvgLengthSq(delta) <= 1.0e-6f)
                {
                    current.points.PopBack();
                }
            }

            current.closed = closed;
            outPaths.PushBack(Move(current));
            current = {};
        }

        void SvgFlattenQuadratic(
            Vector<Point2>& out,
            const Point2& p0,
            const Point2& p1,
            const Point2& p2,
            float toleranceSq,
            uint32_t depth)
        {
            if ((depth >= MaxCubicSubdivisionDepth) || (SvgDistanceToLineSq(p1, p0, p2) <= toleranceSq))
            {
                SvgAppendPointUnique(out, p2);
                return;
            }

            const Point2 p01 = SvgLerpPoint(p0, p1, 0.5f);
            const Point2 p12 = SvgLerpPoint(p1, p2, 0.5f);
            const Point2 p012 = SvgLerpPoint(p01, p12, 0.5f);

            SvgFlattenQuadratic(out, p0, p01, p012, toleranceSq, depth + 1);
            SvgFlattenQuadratic(out, p012, p12, p2, toleranceSq, depth + 1);
        }

        void SvgFlattenCubic(
            Vector<Point2>& out,
            const Point2& p0,
            const Point2& p1,
            const Point2& p2,
            const Point2& p3,
            float toleranceSq,
            uint32_t depth)
        {
            const float d1 = SvgDistanceToLineSq(p1, p0, p3);
            const float d2 = SvgDistanceToLineSq(p2, p0, p3);
            if ((depth >= MaxCubicSubdivisionDepth) || (Max(d1, d2) <= toleranceSq))
            {
                SvgAppendPointUnique(out, p3);
                return;
            }

            const Point2 p01 = SvgLerpPoint(p0, p1, 0.5f);
            const Point2 p12 = SvgLerpPoint(p1, p2, 0.5f);
            const Point2 p23 = SvgLerpPoint(p2, p3, 0.5f);
            const Point2 p012 = SvgLerpPoint(p01, p12, 0.5f);
            const Point2 p123 = SvgLerpPoint(p12, p23, 0.5f);
            const Point2 p0123 = SvgLerpPoint(p012, p123, 0.5f);

            SvgFlattenCubic(out, p0, p01, p012, p0123, toleranceSq, depth + 1);
            SvgFlattenCubic(out, p0123, p123, p23, p3, toleranceSq, depth + 1);
        }

        bool SvgDecodeStrokePaths(
            Vector<SvgStrokePath>& outPaths,
            Span<const StrokeSourcePoint> points,
            Span<const StrokeSourceCommand> commands,
            float flattenTolerance)
        {
            outPaths.Clear();
            if (commands.IsEmpty())
            {
                return false;
            }

            const float toleranceSq = flattenTolerance * flattenTolerance;
            SvgStrokePath currentPath{};
            Point2 currentPoint{};
            bool hasCurrent = false;

            auto readPoint = [&](uint32_t pointIndex, Point2& outPoint) -> bool
            {
                if (pointIndex >= points.Size())
                {
                    return false;
                }

                outPoint.x = points[pointIndex].x;
                outPoint.y = points[pointIndex].y;
                return true;
            };

            for (const StrokeSourceCommand& command : commands)
            {
                const uint32_t pointIndex = command.firstPoint;
                switch (command.type)
                {
                    case StrokeCommandType::MoveTo:
                    {
                        if (!currentPath.points.IsEmpty())
                        {
                            SvgFinalizeStrokePath(outPaths, currentPath, false);
                        }

                        Point2 point{};
                        if (!readPoint(pointIndex, point))
                        {
                            return false;
                        }

                        SvgAppendPointUnique(currentPath.points, point);
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

                        SvgAppendPointUnique(currentPath.points, point);
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

                        SvgFlattenQuadratic(currentPath.points, currentPoint, control, point, toleranceSq, 0);
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

                        SvgFlattenCubic(currentPath.points, currentPoint, control1, control2, point, toleranceSq, 0);
                        currentPoint = point;
                        break;
                    }

                    case StrokeCommandType::Close:
                        if (!currentPath.points.IsEmpty())
                        {
                            SvgFinalizeStrokePath(outPaths, currentPath, true);
                            hasCurrent = false;
                        }
                        break;
                }
            }

            if (!currentPath.points.IsEmpty())
            {
                SvgFinalizeStrokePath(outPaths, currentPath, false);
            }

            return !outPaths.IsEmpty();
        }

        float SvgLength(const Point2& value)
        {
            return Sqrt(SvgLengthSq(value));
        }

        Point2 SvgNormalize(const Point2& value)
        {
            const float lengthSq = SvgLengthSq(value);
            if (lengthSq <= DegenerateLineLengthSq)
            {
                return {};
            }

            const float invLength = 1.0f / Sqrt(lengthSq);
            return value * invLength;
        }

        Point2 SvgLeftNormal(const Point2& direction)
        {
            return { -direction.y, direction.x };
        }

        void SvgEmitRoundCap(
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
            const bool ccw = SvgAngleOnSweepCCW(startAngle, endAngle, exteriorAngle);

            Vector<Point2> polygon{};
            SvgBuildArcPolygon(polygon, center, startPoint, endPoint, radius, ccw);
            SvgEmitClosedPolygon(outCurves, polygon);
        }

        void SvgEmitJoinPatch(
            Vector<CurveData>& outCurves,
            const Point2& vertex,
            const Point2& dirIn,
            const Point2& dirOut,
            float halfWidth,
            const StyleState& style)
        {
            const float turn = SvgCross(dirIn, dirOut);
            if (Abs(turn) <= 1.0e-5f)
            {
                return;
            }

            Point2 offsetIn = SvgLeftNormal(dirIn) * halfWidth;
            Point2 offsetOut = SvgLeftNormal(dirOut) * halfWidth;
            if (turn < 0.0f)
            {
                offsetIn = offsetIn * -1.0f;
                offsetOut = offsetOut * -1.0f;
            }

            const Point2 outerStart = vertex + offsetIn;
            const Point2 outerEnd = vertex + offsetOut;
            Vector<Point2> polygon{};

            switch (style.strokeJoin)
            {
                case StrokeJoinKind::Round:
                    SvgBuildArcPolygon(polygon, vertex, outerStart, outerEnd, halfWidth, turn > 0.0f);
                    SvgEmitClosedPolygon(outCurves, polygon);
                    return;

                case StrokeJoinKind::Miter:
                {
                    Point2 intersection{};
                    if (SvgComputeLineIntersection(intersection, outerStart, dirIn, outerEnd, dirOut))
                    {
                        const float miterLength = SvgLength(intersection - vertex) / Max(halfWidth, 1.0e-5f);
                        if (miterLength <= Max(style.strokeMiterLimit, 1.0f))
                        {
                            polygon.PushBack(vertex);
                            polygon.PushBack(outerStart);
                            polygon.PushBack(intersection);
                            polygon.PushBack(outerEnd);
                            SvgEmitClosedPolygon(outCurves, polygon);
                            return;
                        }
                    }
                    break;
                }

                case StrokeJoinKind::Bevel:
                default:
                    break;
            }

            polygon.PushBack(vertex);
            polygon.PushBack(outerStart);
            polygon.PushBack(outerEnd);
            SvgEmitClosedPolygon(outCurves, polygon);
        }

        void SvgBuildStrokeCurves(
            Vector<CurveData>& outCurves,
            Span<const SvgStrokePath> paths,
            const StyleState& style)
        {
            outCurves.Clear();
            if (style.strokeWidth <= 0.0f)
            {
                return;
            }

            const float halfWidth = style.strokeWidth * 0.5f;
            for (const SvgStrokePath& path : paths)
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
                    const Point2 direction = SvgNormalize(end - start);
                    if (SvgLengthSq(direction) <= 1.0e-6f)
                    {
                        continue;
                    }

                    if (!path.closed && (style.strokeCap == StrokeCapKind::Square))
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
                    else if (!path.closed && (style.strokeCap == StrokeCapKind::Butt))
                    {
                        const bool axisAligned =
                            (Abs(direction.x) <= 1.0e-5f)
                            || (Abs(direction.y) <= 1.0e-5f);
                        if (axisAligned)
                        {
                            // A tiny overlap avoids visible AA seams where SVGs place a filled
                            // arrowhead immediately after a stroked axis line.
                            const float seamOverlap = Min(halfWidth, 0.25f);
                            if (segmentIndex == 0)
                            {
                                start = start - (direction * seamOverlap);
                            }

                            if (segmentIndex == (segmentCount - 1))
                            {
                                end = end + (direction * seamOverlap);
                            }
                        }
                    }

                    const Point2 normal = SvgLeftNormal(direction) * halfWidth;
                    const Point2 polygon[] =
                    {
                        start + normal,
                        start - normal,
                        end - normal,
                        end + normal,
                    };
                    SvgEmitClosedPolygon(outCurves, Span<const Point2>(polygon, HE_LENGTH_OF(polygon)));
                }

                const uint32_t joinStart = path.closed ? 0u : 1u;
                const uint32_t joinEnd = path.closed ? path.points.Size() : (path.points.Size() - 1);
                for (uint32_t pointIndex = joinStart; pointIndex < joinEnd; ++pointIndex)
                {
                    const uint32_t prevIndex = pointIndex == 0 ? (path.points.Size() - 1) : (pointIndex - 1);
                    const uint32_t nextIndex = (pointIndex + 1) % path.points.Size();
                    const Point2 dirIn = SvgNormalize(path.points[pointIndex] - path.points[prevIndex]);
                    const Point2 dirOut = SvgNormalize(path.points[nextIndex] - path.points[pointIndex]);
                    if ((SvgLengthSq(dirIn) <= 1.0e-6f) || (SvgLengthSq(dirOut) <= 1.0e-6f))
                    {
                        continue;
                    }

                    SvgEmitJoinPatch(outCurves, path.points[pointIndex], dirIn, dirOut, halfWidth, style);
                }

                if (!path.closed && (style.strokeCap == StrokeCapKind::Round))
                {
                    const Point2 startDir = SvgNormalize(path.points[1] - path.points[0]);
                    const Point2 startNormal = SvgLeftNormal(startDir) * halfWidth;
                    SvgEmitRoundCap(
                        outCurves,
                        path.points[0],
                        path.points[0] - startNormal,
                        path.points[0] + startNormal,
                        startDir * -1.0f,
                        halfWidth);

                    const uint32_t lastPointIndex = path.points.Size() - 1;
                    const Point2 endDir = SvgNormalize(path.points[lastPointIndex] - path.points[lastPointIndex - 1]);
                    const Point2 endNormal = SvgLeftNormal(endDir) * halfWidth;
                    SvgEmitRoundCap(
                        outCurves,
                        path.points[lastPointIndex],
                        path.points[lastPointIndex] + endNormal,
                        path.points[lastPointIndex] - endNormal,
                        endDir,
                        halfWidth);
                }
            }
        }

        bool BuildAuthoredStrokeShape(
            ParsedShape& out,
            const ParsedShape& source,
            const StyleState& style,
            float strokeMetricScale,
            float flatteningTolerance)
        {
            out = {};

            StrokeOutlineStyle outlineStyle{};
            outlineStyle.width = style.strokeWidth;
            outlineStyle.join = style.strokeJoin;
            outlineStyle.cap = style.strokeCap;
            outlineStyle.miterLimit = style.strokeMiterLimit;

            Vector<StrokeOutlineSourcePoint> outlinePoints{};
            Vector<StrokeOutlineSourceCommand> outlineCommands{};

            Vector<float> dashPattern{};
            const bool hasDashPattern =
                !style.strokeDashArray.IsEmpty()
                && ParseDashArrayScaled(
                    dashPattern,
                    StringView(style.strokeDashArray.Data(), style.strokeDashArray.Size()),
                    strokeMetricScale)
                && !dashPattern.IsEmpty();

            if (hasDashPattern)
            {
                StrokeSourceBuilder dashedBuilder(flatteningTolerance);
                auto appendDashedLine = [&](const Point2& from, const Point2& to)
                {
                    const Point2 delta = to - from;
                    const float lengthSq = (delta.x * delta.x) + (delta.y * delta.y);
                    if (lengthSq <= 1.0e-8f)
                    {
                        return;
                    }

                    const float length = Sqrt(lengthSq);
                    uint32_t dashIndex = 0;
                    float dashConsumed = 0.0f;
                    bool dashOn = true;
                    float segmentStart = 0.0f;

                    while (segmentStart < length)
                    {
                        const float dashLength = dashPattern[dashIndex];
                        const float remainingDash = dashLength - dashConsumed;
                        const float segmentEnd = Min(segmentStart + remainingDash, length);
                        if (dashOn && ((segmentEnd - segmentStart) > 1.0e-5f))
                        {
                            const float t0 = segmentStart / length;
                            const float t1 = segmentEnd / length;
                            const Point2 startPoint{
                                Lerp(from.x, to.x, t0),
                                Lerp(from.y, to.y, t0)
                            };
                            const Point2 endPoint{
                                Lerp(from.x, to.x, t1),
                                Lerp(from.y, to.y, t1)
                            };
                            dashedBuilder.AppendMoveTo(startPoint);
                            dashedBuilder.AppendLineTo(endPoint);
                        }

                        dashConsumed += segmentEnd - segmentStart;
                        segmentStart = segmentEnd;
                        if (dashConsumed >= (dashLength - 1.0e-5f))
                        {
                            dashConsumed = 0.0f;
                            dashIndex = (dashIndex + 1) % dashPattern.Size();
                            dashOn = !dashOn;
                        }
                    }
                };

                Point2 current{};
                Point2 subpathStart{};
                bool hasCurrent = false;
                for (const StrokeSourceCommand& command : source.strokeCommands)
                {
                    switch (command.type)
                    {
                        case StrokeCommandType::MoveTo:
                        {
                            if (command.firstPoint >= source.strokePoints.Size())
                            {
                                return false;
                            }

                            current = {
                                source.strokePoints[command.firstPoint].x,
                                source.strokePoints[command.firstPoint].y
                            };
                            subpathStart = current;
                            hasCurrent = true;
                            break;
                        }

                        case StrokeCommandType::LineTo:
                        {
                            if (!hasCurrent || (command.firstPoint >= source.strokePoints.Size()))
                            {
                                return false;
                            }

                            const Point2 target{
                                source.strokePoints[command.firstPoint].x,
                                source.strokePoints[command.firstPoint].y
                            };
                            appendDashedLine(current, target);
                            current = target;
                            break;
                        }

                        case StrokeCommandType::Close:
                        {
                            if (hasCurrent)
                            {
                                appendDashedLine(current, subpathStart);
                                current = subpathStart;
                            }
                            break;
                        }

                        case StrokeCommandType::QuadraticTo:
                        case StrokeCommandType::CubicTo:
                        default:
                            // Keep current behavior for non-line dashes until we add proper curve-length dash support.
                            dashedBuilder.Clear();
                            dashPattern.Clear();
                            break;
                    }

                    if (dashPattern.IsEmpty())
                    {
                        break;
                    }
                }

                if (!dashPattern.IsEmpty())
                {
                    outlinePoints.Resize(dashedBuilder.Points().Size());
                    for (uint32_t pointIndex = 0; pointIndex < dashedBuilder.Points().Size(); ++pointIndex)
                    {
                        outlinePoints[pointIndex].x = dashedBuilder.Points()[pointIndex].x;
                        outlinePoints[pointIndex].y = dashedBuilder.Points()[pointIndex].y;
                    }

                    outlineCommands.Resize(dashedBuilder.Commands().Size());
                    for (uint32_t commandIndex = 0; commandIndex < dashedBuilder.Commands().Size(); ++commandIndex)
                    {
                        outlineCommands[commandIndex].type = dashedBuilder.Commands()[commandIndex].type;
                        outlineCommands[commandIndex].firstPoint = dashedBuilder.Commands()[commandIndex].firstPoint;
                    }
                }
            }

            if (outlineCommands.IsEmpty())
            {
                outlinePoints.Resize(source.strokePoints.Size());
                for (uint32_t pointIndex = 0; pointIndex < source.strokePoints.Size(); ++pointIndex)
                {
                    outlinePoints[pointIndex].x = source.strokePoints[pointIndex].x;
                    outlinePoints[pointIndex].y = source.strokePoints[pointIndex].y;
                }

                outlineCommands.Resize(source.strokeCommands.Size());
                for (uint32_t commandIndex = 0; commandIndex < source.strokeCommands.Size(); ++commandIndex)
                {
                    outlineCommands[commandIndex].type = source.strokeCommands[commandIndex].type;
                    outlineCommands[commandIndex].firstPoint = source.strokeCommands[commandIndex].firstPoint;
                }
            }

            bool singleOpenLineStroke = (outlineCommands.Size() == 2u)
                && (outlineCommands[0].type == StrokeCommandType::MoveTo)
                && (outlineCommands[1].type == StrokeCommandType::LineTo);

            if (singleOpenLineStroke)
            {
                Vector<StrokeSourcePoint> linePoints{};
                linePoints.Resize(outlinePoints.Size());
                for (uint32_t pointIndex = 0; pointIndex < outlinePoints.Size(); ++pointIndex)
                {
                    linePoints[pointIndex].x = outlinePoints[pointIndex].x;
                    linePoints[pointIndex].y = outlinePoints[pointIndex].y;
                }

                Vector<StrokeSourceCommand> lineCommands{};
                lineCommands.Resize(outlineCommands.Size());
                for (uint32_t commandIndex = 0; commandIndex < outlineCommands.Size(); ++commandIndex)
                {
                    lineCommands[commandIndex].type = outlineCommands[commandIndex].type;
                    lineCommands[commandIndex].firstPoint = outlineCommands[commandIndex].firstPoint;
                }

                Vector<SvgStrokePath> paths{};
                if (!SvgDecodeStrokePaths(
                        paths,
                        Span<const StrokeSourcePoint>(linePoints.Data(), linePoints.Size()),
                        Span<const StrokeSourceCommand>(lineCommands.Data(), lineCommands.Size()),
                        flatteningTolerance))
                {
                    return false;
                }

                SvgBuildStrokeCurves(out.curves, Span<const SvgStrokePath>(paths.Data(), paths.Size()), style);
                if (out.curves.IsEmpty())
                {
                    return false;
                }

                out.fillRule = FillRule::NonZero;
                out.color = style.stroke;
                RecomputeShapeBounds(out);
                return true;
            }

            Vector<StrokeOutlineCurve> strokedCurves{};
            if (!BuildStrokedOutlineCurves(
                    strokedCurves,
                    Span<const StrokeOutlineSourcePoint>(outlinePoints.Data(), outlinePoints.Size()),
                    Span<const StrokeOutlineSourceCommand>(outlineCommands.Data(), outlineCommands.Size()),
                    outlineStyle))
            {
                return false;
            }

            curve_compile::CurveBuilder builder(flatteningTolerance);
            for (const StrokeOutlineCurve& curve : strokedCurves)
            {
                const Point2 p0{ curve.x0, curve.y0 };
                if (curve.kind == StrokeOutlineCurveKind::Line)
                {
                    builder.AddLine(p0, { curve.x1, curve.y1 });
                }
                else if (curve.kind == StrokeOutlineCurveKind::Quadratic)
                {
                    builder.AddQuadratic(p0, { curve.x1, curve.y1 }, { curve.x2, curve.y2 });
                }
                else
                {
                    builder.AddCubic(p0, { curve.x1, curve.y1 }, { curve.x2, curve.y2 }, { curve.x3, curve.y3 });
                }
            }

            out.curves = Move(builder.Curves());
            if (out.curves.IsEmpty())
            {
                return false;
            }
            out.fillRule = FillRule::NonZero;
            out.color = style.stroke;
            RecomputeShapeBounds(out);
            return true;
        }

        bool IsLineOnlyStrokeSource(const ParsedShape& source)
        {
            for (const StrokeSourceCommand& command : source.strokeCommands)
            {
                if ((command.type != StrokeCommandType::MoveTo)
                    && (command.type != StrokeCommandType::LineTo)
                    && (command.type != StrokeCommandType::Close))
                {
                    return false;
                }
            }

            return !source.strokeCommands.IsEmpty();
        }

        bool BuildAuthoredDashedLineStrokeShapes(
            Vector<ParsedShape>& outShapes,
            const ParsedShape& source,
            const StyleState& style,
            float strokeMetricScale,
            float flatteningTolerance)
        {
            outShapes.Clear();

            Vector<float> dashPattern{};
            if (style.strokeDashArray.IsEmpty()
                || !ParseDashArrayScaled(
                    dashPattern,
                    StringView(style.strokeDashArray.Data(), style.strokeDashArray.Size()),
                    strokeMetricScale)
                || dashPattern.IsEmpty())
            {
                return false;
            }

            StrokeSourceBuilder dashedBuilder(flatteningTolerance);
            auto appendDashedLine = [&](const Point2& from, const Point2& to)
            {
                const Point2 delta = to - from;
                const float lengthSq = (delta.x * delta.x) + (delta.y * delta.y);
                if (lengthSq <= 1.0e-8f)
                {
                    return;
                }

                const float length = Sqrt(lengthSq);
                uint32_t dashIndex = 0;
                float dashConsumed = 0.0f;
                bool dashOn = true;
                float segmentStart = 0.0f;

                while (segmentStart < length)
                {
                    const float dashLength = dashPattern[dashIndex];
                    const float remainingDash = dashLength - dashConsumed;
                    const float segmentEnd = Min(segmentStart + remainingDash, length);
                    if (dashOn && ((segmentEnd - segmentStart) > 1.0e-5f))
                    {
                        const float t0 = segmentStart / length;
                        const float t1 = segmentEnd / length;
                        const Point2 startPoint{
                            Lerp(from.x, to.x, t0),
                            Lerp(from.y, to.y, t0)
                        };
                        const Point2 endPoint{
                            Lerp(from.x, to.x, t1),
                            Lerp(from.y, to.y, t1)
                        };
                        dashedBuilder.AppendMoveTo(startPoint);
                        dashedBuilder.AppendLineTo(endPoint);
                    }

                    dashConsumed += segmentEnd - segmentStart;
                    segmentStart = segmentEnd;
                    if (dashConsumed >= (dashLength - 1.0e-5f))
                    {
                        dashConsumed = 0.0f;
                        dashIndex = (dashIndex + 1) % dashPattern.Size();
                        dashOn = !dashOn;
                    }
                }
            };

            Point2 current{};
            Point2 subpathStart{};
            bool hasCurrent = false;
            for (const StrokeSourceCommand& command : source.strokeCommands)
            {
                switch (command.type)
                {
                    case StrokeCommandType::MoveTo:
                    {
                        if (command.firstPoint >= source.strokePoints.Size())
                        {
                            return false;
                        }

                        current = {
                            source.strokePoints[command.firstPoint].x,
                            source.strokePoints[command.firstPoint].y
                        };
                        subpathStart = current;
                        hasCurrent = true;
                        break;
                    }

                    case StrokeCommandType::LineTo:
                    {
                        if (!hasCurrent || (command.firstPoint >= source.strokePoints.Size()))
                        {
                            return false;
                        }

                        const Point2 target{
                            source.strokePoints[command.firstPoint].x,
                            source.strokePoints[command.firstPoint].y
                        };
                        appendDashedLine(current, target);
                        current = target;
                        break;
                    }

                    case StrokeCommandType::Close:
                    {
                        if (hasCurrent)
                        {
                            appendDashedLine(current, subpathStart);
                            current = subpathStart;
                        }
                        break;
                    }

                    default:
                        return false;
                }
            }

            Vector<StrokeSourcePoint> dashedPoints = Move(dashedBuilder.Points());
            Vector<StrokeSourceCommand> dashedCommands = Move(dashedBuilder.Commands());
            Vector<SvgStrokePath> paths{};
            if (!SvgDecodeStrokePaths(
                    paths,
                    Span<const StrokeSourcePoint>(dashedPoints.Data(), dashedPoints.Size()),
                    Span<const StrokeSourceCommand>(dashedCommands.Data(), dashedCommands.Size()),
                    flatteningTolerance))
            {
                return false;
            }

            outShapes.Reserve(paths.Size());
            for (uint32_t pathIndex = 0; pathIndex < paths.Size(); ++pathIndex)
            {
                ParsedShape& dashShape = outShapes.EmplaceBack();
                SvgBuildStrokeCurves(dashShape.curves, Span<const SvgStrokePath>(&paths[pathIndex], 1), style);
                if (dashShape.curves.IsEmpty())
                {
                    outShapes.Resize(outShapes.Size() - 1);
                    continue;
                }

                dashShape.fillRule = FillRule::NonZero;
                dashShape.color = style.stroke;
                RecomputeShapeBounds(dashShape);
            }

            return !outShapes.IsEmpty();
        }

        uint32_t AppendParsedShape(ParsedImage& out, ParsedShape&& shape)
        {
            ParsedShape& outShape = out.shapes.EmplaceBack();
            outShape = Move(shape);

            if (out.shapes.Size() == 1)
            {
                out.boundsMinX = outShape.minX;
                out.boundsMinY = outShape.minY;
                out.boundsMaxX = outShape.maxX;
                out.boundsMaxY = outShape.maxY;
            }
            else
            {
                out.boundsMinX = Min(out.boundsMinX, outShape.minX);
                out.boundsMinY = Min(out.boundsMinY, outShape.minY);
                out.boundsMaxX = Max(out.boundsMaxX, outShape.maxX);
                out.boundsMaxY = Max(out.boundsMaxY, outShape.maxY);
            }

            return out.shapes.Size() - 1;
        }

        class CurveBuilder final
        {
        public:
            explicit CurveBuilder(float flatteningTolerance)
                : m_builder(flatteningTolerance)
            {
            }

            void Clear()
            {
                m_builder.Clear();
            }

            void SetTransform(const Affine2D& transform)
            {
                m_builder.SetTransform(transform);
            }

            void AddLine(const Point2& from, const Point2& to)
            {
                m_builder.AddLine(from, to);
            }

            void AddQuadratic(const Point2& p1, const Point2& p2, const Point2& p3)
            {
                m_builder.AddQuadratic(p1, p2, p3);
            }

            void AddCubic(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3)
            {
                m_builder.AddCubic(p0, p1, p2, p3);
            }

            Vector<CurveData>& Curves() { return m_builder.Curves(); }

        private:
            curve_compile::CurveBuilder m_builder;
        };

        class FreeTypeLibrary final
        {
        public:
            ~FreeTypeLibrary() noexcept
            {
                if (m_library != nullptr)
                {
                    FT_Done_FreeType(m_library);
                }
            }

            bool Initialize()
            {
                const FT_Error err = FT_Init_FreeType(&m_library);
                if (err != 0)
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to initialize FreeType for SVG text compilation."),
                        HE_KV(error, err));
                    return false;
                }

                return true;
            }

            FT_Library Get() const { return m_library; }

        private:
            FT_Library m_library{ nullptr };
        };

        class FreeTypeFace final
        {
        public:
            ~FreeTypeFace() noexcept
            {
                if (m_face != nullptr)
                {
                    FT_Done_Face(m_face);
                }
            }

            bool Load(FT_Library library, Span<const uint8_t> sourceBytes, uint32_t faceIndex)
            {
                const FT_Error err = FT_New_Memory_Face(
                    library,
                    reinterpret_cast<const FT_Byte*>(sourceBytes.Data()),
                    static_cast<FT_Long>(sourceBytes.Size()),
                    static_cast<FT_Long>(faceIndex),
                    &m_face);
                if (err != 0)
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to load FreeType face for SVG text compilation."),
                        HE_KV(face_index, faceIndex),
                        HE_KV(error, err));
                    return false;
                }

                return true;
            }

            FT_Face Get() const { return m_face; }

        private:
            FT_Face m_face{ nullptr };
        };

        Point2 ToPoint(const FT_Vector& value)
        {
            return {
                static_cast<float>(value.x),
                static_cast<float>(value.y)
            };
        }

        class SvgTextOutlineBuilder final
        {
        public:
            explicit SvgTextOutlineBuilder(float cubicTolerance)
                : m_curveBuilder(cubicTolerance)
                , m_strokeBuilder(cubicTolerance)
            {
            }

            bool AppendGlyph(
                Vector<CurveData>& outCurves,
                Vector<StrokeSourcePoint>& outPoints,
                Vector<StrokeSourceCommand>& outCommands,
                const FT_Outline& outline,
                const Affine2D& transform)
            {
                m_transform = transform;
                m_curveBuilder.Clear();
                m_strokeBuilder.Clear();
                m_hasContour = false;
                m_current = {};

                FT_Outline_Funcs funcs{};
                funcs.move_to = &MoveTo;
                funcs.line_to = &LineTo;
                funcs.conic_to = &ConicTo;
                funcs.cubic_to = &CubicTo;

                const FT_Error err = FT_Outline_Decompose(const_cast<FT_Outline*>(&outline), &funcs, this);
                if (err != 0)
                {
                    return false;
                }

                if (m_hasContour)
                {
                    m_strokeBuilder.AppendClose();
                }

                outCurves.Insert(outCurves.Size(), m_curveBuilder.Curves().Data(), m_curveBuilder.Curves().Size());
                AppendStrokeSourceData(
                    outPoints,
                    outCommands,
                    Span<const StrokeSourcePoint>(m_strokeBuilder.Points().Data(), m_strokeBuilder.Points().Size()),
                    Span<const StrokeSourceCommand>(m_strokeBuilder.Commands().Data(), m_strokeBuilder.Commands().Size()));
                return true;
            }

        private:
            Point2 TransformOutlinePoint(const FT_Vector& value) const
            {
                return TransformPoint(m_transform, ToPoint(value));
            }

            static int MoveTo(const FT_Vector* to, void* user)
            {
                SvgTextOutlineBuilder& self = *static_cast<SvgTextOutlineBuilder*>(user);
                if (self.m_hasContour)
                {
                    self.m_strokeBuilder.AppendClose();
                }

                self.m_current = self.TransformOutlinePoint(*to);
                self.m_strokeBuilder.AppendMoveTo(self.m_current);
                self.m_hasContour = true;
                return 0;
            }

            static int LineTo(const FT_Vector* to, void* user)
            {
                SvgTextOutlineBuilder& self = *static_cast<SvgTextOutlineBuilder*>(user);
                const Point2 target = self.TransformOutlinePoint(*to);
                self.m_curveBuilder.AddLine(self.m_current, target);
                self.m_strokeBuilder.AppendLineTo(target);
                self.m_current = target;
                return 0;
            }

            static int ConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
            {
                SvgTextOutlineBuilder& self = *static_cast<SvgTextOutlineBuilder*>(user);
                const Point2 c = self.TransformOutlinePoint(*control);
                const Point2 target = self.TransformOutlinePoint(*to);
                self.m_curveBuilder.AddQuadratic(self.m_current, c, target);
                self.m_strokeBuilder.AppendQuadraticTo(c, target);
                self.m_current = target;
                return 0;
            }

            static int CubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
            {
                SvgTextOutlineBuilder& self = *static_cast<SvgTextOutlineBuilder*>(user);
                const Point2 c1 = self.TransformOutlinePoint(*control1);
                const Point2 c2 = self.TransformOutlinePoint(*control2);
                const Point2 target = self.TransformOutlinePoint(*to);
                self.m_curveBuilder.AddCubic(self.m_current, c1, c2, target);
                self.m_strokeBuilder.AppendCubicTo(c1, c2, target);
                self.m_current = target;
                return 0;
            }

        private:
            CurveBuilder m_curveBuilder;
            StrokeSourceBuilder m_strokeBuilder;
            Affine2D m_transform{};
            Point2 m_current{};
            bool m_hasContour{ false };
        };

        [[maybe_unused]] void ZeroCounts(Vector<uint32_t>& counts)
        {
            for (uint32_t i = 0; i < counts.Size(); ++i)
            {
                counts[i] = 0;
            }
        }

        [[maybe_unused]] void GetBandRange(
            int32_t& outStart,
            int32_t& outEnd,
            float curveMin,
            float curveMax,
            float boundsMin,
            float boundsSpan,
            uint32_t bandCount,
            float epsilon)
        {
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

        uint32_t SvgChooseBandCount(
            const Vector<CurveData>& curves,
            bool horizontalBands,
            float boundsMin,
            float boundsMax,
            float epsilon)
        {
            return curve_compile::ChooseBandCount(curves, horizontalBands, boundsMin, boundsMax, epsilon);
        }

        void SvgBuildBandRefs(
            Vector<Vector<CurveRef>>& outBands,
            const Vector<CurveData>& curves,
            bool horizontalBands,
            float boundsMin,
            float boundsMax,
            uint32_t bandCount,
            float epsilon)
        {
            curve_compile::BuildBandRefs(outBands, curves, horizontalBands, boundsMin, boundsMax, bandCount, epsilon);
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

        bool IsWhitespace(char c)
        {
            return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') || (c == '\f');
        }

        bool IsNameChar(char c)
        {
            return ((c >= 'a') && (c <= 'z'))
                || ((c >= 'A') && (c <= 'Z'))
                || ((c >= '0') && (c <= '9'))
                || (c == '_')
                || (c == '-')
                || (c == ':');
        }

        StringView TrimView(StringView value)
        {
            uint32_t begin = 0;
            uint32_t end = value.Size();
            while ((begin < end) && IsWhitespace(value[begin]))
            {
                ++begin;
            }

            while ((end > begin) && IsWhitespace(value[end - 1]))
            {
                --end;
            }

            return { value.Data() + begin, end - begin };
        }

        StringView TrimQuotes(StringView value)
        {
            value = TrimView(value);
            if ((value.Size() >= 2) && ((value[0] == '"') || (value[0] == '\'')) && (value[value.Size() - 1] == value[0]))
            {
                return value.Substring(1, value.Size() - 2);
            }

            return value;
        }

        bool ResolveRepoFontPath(String& out, const char* fileName)
        {
            static const char* Candidates[] =
            {
                "plugins/editor/src/fonts/",
                "../../../plugins/editor/src/fonts/",
            };

            for (const char* base : Candidates)
            {
                out = base;
                out += fileName;
                if (File::Exists(out.Data()))
                {
                    return true;
                }
            }

            out.Clear();
            return false;
        }

        StringView GetPrimaryFontFamilyName(StringView value)
        {
            value = TrimView(value);
            if (const char* comma = value.Find(','))
            {
                value = value.Substring(0, static_cast<uint32_t>(comma - value.Data()));
            }

            return TrimQuotes(value);
        }

        StringView StripSvgSubsetFontPrefix(StringView familyName)
        {
            if (const char* plus = familyName.Find('+'))
            {
                familyName = familyName.Substring(static_cast<uint32_t>((plus - familyName.Data()) + 1));
            }

            return familyName;
        }

        String BuildSvgFontKey(StringView familyName, StringView fontStyle, StringView fontWeight)
        {
            familyName = GetPrimaryFontFamilyName(familyName);
            familyName = StripSvgSubsetFontPrefix(familyName);
            fontStyle = TrimQuotes(TrimView(fontStyle));
            fontWeight = TrimQuotes(TrimView(fontWeight));

            if (familyName.IsEmpty()
                || familyName.EqualToI("sans-serif")
                || familyName.EqualToI("sans")
                || familyName.EqualToI("Noto Sans"))
            {
                return "Noto Sans";
            }

            if (familyName.EqualToI("monospace")
                || familyName.EqualToI("mono")
                || familyName.EqualToI("Noto Mono"))
            {
                return "Noto Mono";
            }

            if (familyName.EqualToI("Times New Roman"))
            {
                const bool wantsItalic = fontStyle.EqualToI("italic");
                const bool wantsBold = fontWeight.EqualToI("bold");
                if (wantsBold && wantsItalic) { return "TimesNewRomanPS-BoldItalicMT"; }
                if (wantsBold) { return "TimesNewRomanPS-BoldMT"; }
                if (wantsItalic) { return "TimesNewRomanPS-ItalicMT"; }
                return "TimesNewRomanPSMT";
            }

            if (familyName.EqualToI("TimesNewRomanPS-BoldItal"))
            {
                return "TimesNewRomanPS-BoldItalicMT";
            }

            if (familyName.EqualToI("Arial") || familyName.EqualToI("ArialMT"))
            {
                return "ArialMT";
            }

            if (familyName.EqualToI("Arial-BoldMT"))
            {
                return "Arial-BoldMT";
            }

            if (familyName.EqualToI("Consolas"))
            {
                return "Consolas";
            }

            if (familyName.EqualToI("SymbolMT"))
            {
                return "SymbolMT";
            }

            return String(familyName);
        }

        uint32_t FindOrAppendSvgFontFace(Vector<CompiledVectorImageFontFaceEntry>& out, StringView key)
        {
            for (uint32_t fontIndex = 0; fontIndex < out.Size(); ++fontIndex)
            {
                if (StringView(out[fontIndex].key.Data(), out[fontIndex].key.Size()).EqualToI(key))
                {
                    return fontIndex;
                }
            }

            CompiledVectorImageFontFaceEntry& entry = out.EmplaceBack();
            entry.key = String(key);
            return out.Size() - 1;
        }

        void ApplyAffineToTextRun(CompiledVectorImageTextRunEntry& run, const Affine2D& transform)
        {
            run.transformX = { transform.m00, transform.m10 };
            run.transformY = { transform.m01, transform.m11 };
            run.transformTranslation = { transform.tx, transform.ty };
        }

        bool ParseSvgNumericEntity(uint32_t& outCodePoint, StringView text, uint32_t base)
        {
            if (text.IsEmpty())
            {
                return false;
            }

            uint32_t value = 0;
            for (char ch : text)
            {
                uint32_t digit = 0;
                if ((ch >= '0') && (ch <= '9'))
                {
                    digit = static_cast<uint32_t>(ch - '0');
                }
                else if ((base == 16) && (ch >= 'a') && (ch <= 'f'))
                {
                    digit = 10u + static_cast<uint32_t>(ch - 'a');
                }
                else if ((base == 16) && (ch >= 'A') && (ch <= 'F'))
                {
                    digit = 10u + static_cast<uint32_t>(ch - 'A');
                }
                else
                {
                    return false;
                }

                if (digit >= base)
                {
                    return false;
                }

                value = (value * base) + digit;
            }

            if (value > 0x10FFFFu)
            {
                return false;
            }

            outCodePoint = value;
            return true;
        }

        bool TryDecodeSvgEntity(String& out, StringView entity)
        {
            if (entity.IsEmpty())
            {
                return false;
            }

            if (entity[0] == '#')
            {
                uint32_t codePoint = InvalidCodePoint;
                if ((entity.Size() > 2) && ((entity[1] == 'x') || (entity[1] == 'X')))
                {
                    if (!ParseSvgNumericEntity(codePoint, entity.Substring(2), 16))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!ParseSvgNumericEntity(codePoint, entity.Substring(1), 10))
                    {
                        return false;
                    }
                }

                UTF8Encode(out, codePoint);
                return true;
            }

            if (entity.EqualTo("amp"))
            {
                out.PushBack('&');
                return true;
            }

            if (entity.EqualTo("lt"))
            {
                out.PushBack('<');
                return true;
            }

            if (entity.EqualTo("gt"))
            {
                out.PushBack('>');
                return true;
            }

            if (entity.EqualTo("quot"))
            {
                out.PushBack('"');
                return true;
            }

            if (entity.EqualTo("apos"))
            {
                out.PushBack('\'');
                return true;
            }

            return false;
        }

        void AppendDecodedSvgText(String& out, StringView rawText)
        {
            const char* cur = rawText.Begin();
            const char* end = rawText.End();
            while (cur < end)
            {
                if (*cur != '&')
                {
                    out.PushBack(*cur);
                    ++cur;
                    continue;
                }

                const char* entityBegin = cur + 1;
                const char* entityEnd = entityBegin;
                while ((entityEnd < end) && (*entityEnd != ';') && (*entityEnd != '&') && (*entityEnd != '<'))
                {
                    ++entityEnd;
                }

                if ((entityEnd >= end) || (*entityEnd != ';'))
                {
                    out.PushBack(*cur);
                    ++cur;
                    continue;
                }

                String entityText{};
                entityText += StringView(entityBegin, static_cast<uint32_t>(entityEnd - entityBegin));
                if (!TryDecodeSvgEntity(out, StringView(entityText.Data(), entityText.Size())))
                {
                    out.PushBack(*cur);
                    ++cur;
                    continue;
                }

                cur = entityEnd + 1;
            }
        }

        bool ShouldPreserveSvgTextWhitespace(Span<const Attribute> attrs, bool defaultValue)
        {
            for (const Attribute& attr : attrs)
            {
                if (!attr.name.EqualToI("xml:space"))
                {
                    continue;
                }

                const StringView value = TrimQuotes(TrimView(attr.value));
                if (value.EqualToI("preserve"))
                {
                    return true;
                }

                if (value.EqualToI("default"))
                {
                    return false;
                }
            }

            return defaultValue;
        }

        bool ResolveWindowsFontPath(String& out, const char* fileName)
        {
            String path = "C:/Windows/Fonts/";
            path += fileName;
            if (!File::Exists(path.Data()))
            {
                return false;
            }

            out = path;
            return true;
        }

        bool ResolveSvgFontPath(String& out, StringView familyName, StringView fontStyle, StringView fontWeight)
        {
            familyName = GetPrimaryFontFamilyName(familyName);
            familyName = StripSvgSubsetFontPrefix(familyName);
            fontStyle = TrimQuotes(TrimView(fontStyle));
            fontWeight = TrimQuotes(TrimView(fontWeight));

            const bool wantsItalic = fontStyle.EqualToI("italic")
                || familyName.EqualToI("TimesNewRomanPS-ItalicMT")
                || familyName.EqualToI("TimesNewRomanPS-BoldItalicMT");
            const bool wantsBold = fontWeight.EqualToI("bold")
                || familyName.EqualToI("TimesNewRomanPS-BoldMT")
                || familyName.EqualToI("TimesNewRomanPS-BoldItalicMT");

            if (familyName.IsEmpty()
                || familyName.EqualToI("sans-serif")
                || familyName.EqualToI("sans")
                || familyName.EqualToI("Noto Sans"))
            {
                if (ResolveRepoFontPath(out, "NotoSans-Regular.ttf"))
                {
                    return true;
                }
            }

            if (familyName.EqualToI("monospace")
                || familyName.EqualToI("mono")
                || familyName.EqualToI("Noto Mono"))
            {
                if (ResolveRepoFontPath(out, "NotoMono-Regular.ttf"))
                {
                    return true;
                }
            }

            if (familyName.EqualToI("Times New Roman")
                || familyName.EqualToI("TimesNewRomanPSMT")
                || familyName.EqualToI("TimesNewRomanPS-ItalicMT")
                || familyName.EqualToI("TimesNewRomanPS-BoldMT")
                || familyName.EqualToI("TimesNewRomanPS-BoldItalicMT"))
            {
                const char* fileName = wantsBold
                    ? (wantsItalic ? "timesbi.ttf" : "timesbd.ttf")
                    : (wantsItalic ? "timesi.ttf" : "times.ttf");
                if (ResolveWindowsFontPath(out, fileName))
                {
                    return true;
                }
            }

            if (familyName.EqualToI("SymbolMT"))
            {
                if (ResolveWindowsFontPath(out, "symbol.ttf"))
                {
                    return true;
                }
            }

            if (familyName.EqualToI("Material Design Icons"))
            {
                if (ResolveRepoFontPath(out, "materialdesignicons.ttf"))
                {
                    return true;
                }
            }

            static const char* WindowsCandidates[] =
            {
                "C:/Windows/Fonts/segoeui.ttf",
                "C:/Windows/Fonts/segoeuib.ttf",
                "C:/Windows/Fonts/segoeuii.ttf",
                "C:/Windows/Fonts/segoeuiz.ttf",
                "C:/Windows/Fonts/arial.ttf",
                "C:/Windows/Fonts/arialbd.ttf",
                "C:/Windows/Fonts/ariali.ttf",
                "C:/Windows/Fonts/arialbi.ttf",
                "C:/Windows/Fonts/calibri.ttf",
                "C:/Windows/Fonts/tahoma.ttf",
            };
            for (const char* candidate : WindowsCandidates)
            {
                if (File::Exists(candidate))
                {
                    out = candidate;
                    return true;
                }
            }

            out.Clear();
            return false;
        }

        bool ParseFloat(StringView text, float& out)
        {
            const char* begin = text.Data();
            const char* end = text.Data() + text.Size();
            const char* parseEnd = end;
            return StrToFloat(out, begin, &parseEnd) && (parseEnd == end);
        }

        bool ParseDashArray(Vector<float>& out, StringView text)
        {
            text = TrimView(text);
            if (text.IsEmpty() || text.EqualToI("none"))
            {
                out.Clear();
                return true;
            }

            out.Clear();
            const char* cur = text.Data();
            const char* endText = text.Data() + text.Size();
            while (cur < endText)
            {
                while ((cur < endText) && (IsWhitespace(*cur) || (*cur == ',')))
                {
                    ++cur;
                }

                if (cur >= endText)
                {
                    break;
                }

                float value = 0.0f;
                const char* parseEnd = endText;
                if (!StrToFloat(value, cur, &parseEnd) || (parseEnd == cur))
                {
                    return false;
                }

                out.PushBack(value);
                cur = parseEnd;
            }

            for (int32_t index = static_cast<int32_t>(out.Size()) - 1; index >= 0; --index)
            {
                if (out[static_cast<uint32_t>(index)] <= 0.0f)
                {
                    out.Erase(static_cast<uint32_t>(index));
                }
            }

            if ((out.Size() & 1u) != 0u)
            {
                const uint32_t originalCount = out.Size();
                out.Reserve(originalCount * 2u);
                for (uint32_t index = 0; index < originalCount; ++index)
                {
                    out.PushBack(out[index]);
                }
            }

            return true;
        }

        bool ParseNumberList(Vector<float>& out, StringView text)
        {
            out.Clear();
            const char* cur = text.Data();
            const char* endText = text.Data() + text.Size();

            while (cur < endText)
            {
                while ((cur < endText) && (IsWhitespace(*cur) || (*cur == ',')))
                {
                    ++cur;
                }

                if (cur >= endText)
                {
                    break;
                }

                float value = 0.0f;
                const char* parseEnd = endText;
                if (!StrToFloat(value, cur, &parseEnd) || (parseEnd == cur))
                {
                    return false;
                }

                out.PushBack(value);
                cur = parseEnd;
            }

            return true;
        }

        bool ParseColor(Vec4f& out, StringView text)
        {
            const StringView trimmed = TrimView(text);
            if (trimmed.IsEmpty())
            {
                return false;
            }

            if (trimmed.EqualToI("none"))
            {
                out = { 0.0f, 0.0f, 0.0f, 0.0f };
                return true;
            }

            if (trimmed[0] == '#')
            {
                auto HexValue = [](char c) -> int32_t
                {
                    if ((c >= '0') && (c <= '9'))
                    {
                        return c - '0';
                    }

                    if ((c >= 'a') && (c <= 'f'))
                    {
                        return 10 + (c - 'a');
                    }

                    if ((c >= 'A') && (c <= 'F'))
                    {
                        return 10 + (c - 'A');
                    }

                    return -1;
                };

                auto ParseByte = [&](char a, char b, float& value) -> bool
                {
                    const int32_t high = HexValue(a);
                    const int32_t low = HexValue(b);
                    if ((high < 0) || (low < 0))
                    {
                        return false;
                    }

                    value = static_cast<float>((high << 4) | low) / 255.0f;
                    return true;
                };

                if ((trimmed.Size() == 7) || (trimmed.Size() == 9))
                {
                    out.w = 1.0f;
                    return ParseByte(trimmed[1], trimmed[2], out.x)
                        && ParseByte(trimmed[3], trimmed[4], out.y)
                        && ParseByte(trimmed[5], trimmed[6], out.z)
                        && ((trimmed.Size() == 7) || ParseByte(trimmed[7], trimmed[8], out.w));
                }
            }

            if (trimmed.EqualToI("black"))
            {
                out = { 0.0f, 0.0f, 0.0f, 1.0f };
                return true;
            }

            if (trimmed.EqualToI("white"))
            {
                out = { 1.0f, 1.0f, 1.0f, 1.0f };
                return true;
            }

            return false;
        }

        bool ParseTransform(Affine2D& out, StringView text)
        {
            out = {};

            const char* cur = text.Data();
            const char* end = text.Data() + text.Size();
            Vector<float> values{};

            while (cur < end)
            {
                while ((cur < end) && IsWhitespace(*cur))
                {
                    ++cur;
                }

                if (cur >= end)
                {
                    break;
                }

                const char* nameBegin = cur;
                while ((cur < end) && IsNameChar(*cur))
                {
                    ++cur;
                }

                const StringView name{ nameBegin, static_cast<uint32_t>(cur - nameBegin) };
                while ((cur < end) && IsWhitespace(*cur))
                {
                    ++cur;
                }

                if ((cur >= end) || (*cur != '('))
                {
                    return false;
                }

                ++cur;
                const char* argsBegin = cur;
                int32_t depth = 1;
                while ((cur < end) && (depth > 0))
                {
                    if (*cur == '(')
                    {
                        ++depth;
                    }
                    else if (*cur == ')')
                    {
                        --depth;
                    }

                    if (depth > 0)
                    {
                        ++cur;
                    }
                }

                if (depth != 0)
                {
                    return false;
                }

                const StringView args{ argsBegin, static_cast<uint32_t>(cur - argsBegin) };
                ++cur;

                if (!ParseNumberList(values, args))
                {
                    return false;
                }

                Affine2D next{};
                if (name.EqualToI("translate"))
                {
                    if (values.IsEmpty() || (values.Size() > 2))
                    {
                        return false;
                    }

                    next = MakeTranslateAffine(values[0], values.Size() > 1 ? values[1] : 0.0f);
                }
                else if (name.EqualToI("scale"))
                {
                    if (values.IsEmpty() || (values.Size() > 2))
                    {
                        return false;
                    }

                    next = MakeScaleAffine(values[0], values.Size() > 1 ? values[1] : values[0]);
                }
                else if (name.EqualToI("rotate"))
                {
                    if ((values.Size() != 1) && (values.Size() != 3))
                    {
                        return false;
                    }

                    next = (values.Size() == 1)
                        ? MakeRotateAffine(values[0])
                        : MakeRotateAffine(values[0], values[1], values[2]);
                }
                else if (name.EqualToI("skewX"))
                {
                    if (values.Size() != 1)
                    {
                        return false;
                    }

                    next = MakeSkewXAffine(values[0]);
                }
                else if (name.EqualToI("skewY"))
                {
                    if (values.Size() != 1)
                    {
                        return false;
                    }

                    next = MakeSkewYAffine(values[0]);
                }
                else if (name.EqualToI("matrix"))
                {
                    if (values.Size() != 6)
                    {
                        return false;
                    }

                    next.m00 = values[0];
                    next.m10 = values[1];
                    next.m01 = values[2];
                    next.m11 = values[3];
                    next.tx = values[4];
                    next.ty = values[5];
                }
                else
                {
                    return false;
                }

                out = ComposeAffine(out, next);
            }

            return true;
        }

        void ApplyStyleProperty(StyleState& style, StringView name, StringView value)
        {
            const StringView trimmedName = TrimView(name);
            const StringView trimmedValue = TrimView(value);

            if (trimmedName.EqualToI("fill"))
            {
                Vec4f color{};
                if (ParseColor(color, trimmedValue))
                {
                    style.fill = color;
                    style.fillNone = (color.w == 0.0f);
                }
            }
            else if (trimmedName.EqualToI("stroke"))
            {
                Vec4f color{};
                if (ParseColor(color, trimmedValue))
                {
                    style.stroke = color;
                    style.strokeNone = (color.w == 0.0f);
                }
            }
            else if (trimmedName.EqualToI("fill-rule"))
            {
                if (trimmedValue.EqualToI("evenodd"))
                {
                    style.fillRule = FillRule::EvenOdd;
                }
                else if (trimmedValue.EqualToI("nonzero"))
                {
                    style.fillRule = FillRule::NonZero;
                }
            }
            else if (trimmedName.EqualToI("stroke-width"))
            {
                float width = 1.0f;
                if (ParseFloat(trimmedValue, width))
                {
                    style.strokeWidth = Max(width, 0.0f);
                }
            }
            else if (trimmedName.EqualToI("stroke-linejoin"))
            {
                if (trimmedValue.EqualToI("round"))
                {
                    style.strokeJoin = StrokeJoinKind::Round;
                }
                else if (trimmedValue.EqualToI("bevel"))
                {
                    style.strokeJoin = StrokeJoinKind::Bevel;
                }
                else
                {
                    style.strokeJoin = StrokeJoinKind::Miter;
                }
            }
            else if (trimmedName.EqualToI("stroke-linecap"))
            {
                if (trimmedValue.EqualToI("round"))
                {
                    style.strokeCap = StrokeCapKind::Round;
                }
                else if (trimmedValue.EqualToI("square"))
                {
                    style.strokeCap = StrokeCapKind::Square;
                }
                else
                {
                    style.strokeCap = StrokeCapKind::Butt;
                }
            }
            else if (trimmedName.EqualToI("stroke-miterlimit"))
            {
                float miterLimit = 4.0f;
                if (ParseFloat(trimmedValue, miterLimit))
                {
                    style.strokeMiterLimit = Max(miterLimit, 0.0f);
                }
            }
            else if (trimmedName.EqualToI("stroke-dasharray"))
            {
                style.strokeDashArray = String(trimmedValue);
            }
            else if (trimmedName.EqualToI("opacity"))
            {
                float alpha = 1.0f;
                if (ParseFloat(trimmedValue, alpha))
                {
                    alpha = Clamp(alpha, 0.0f, 1.0f);
                    style.fill.w *= alpha;
                    style.stroke.w *= alpha;
                    style.fillNone = (style.fill.w <= 0.0f);
                    style.strokeNone = (style.stroke.w <= 0.0f);
                }
            }
            else if (trimmedName.EqualToI("fill-opacity"))
            {
                float alpha = 1.0f;
                if (ParseFloat(trimmedValue, alpha))
                {
                    style.fill.w *= Clamp(alpha, 0.0f, 1.0f);
                    style.fillNone = (style.fill.w <= 0.0f);
                }
            }
            else if (trimmedName.EqualToI("stroke-opacity"))
            {
                float alpha = 1.0f;
                if (ParseFloat(trimmedValue, alpha))
                {
                    style.stroke.w *= Clamp(alpha, 0.0f, 1.0f);
                    style.strokeNone = (style.stroke.w <= 0.0f);
                }
            }
        }

        void ApplyPresentationAttributes(ParseState& state, Span<const Attribute> attrs)
        {
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("transform"))
                {
                    Affine2D transform{};
                    if (ParseTransform(transform, attr.value))
                    {
                        state.transform = ComposeAffine(state.transform, transform);
                    }
                }
                else if (attr.name.EqualToI("style"))
                {
                    const char* cur = attr.value.Data();
                    const char* end = attr.value.Data() + attr.value.Size();
                    while (cur < end)
                    {
                        const char* nameBegin = cur;
                        while ((cur < end) && (*cur != ':') && (*cur != ';'))
                        {
                            ++cur;
                        }

                        const StringView name{ nameBegin, static_cast<uint32_t>(cur - nameBegin) };
                        if ((cur >= end) || (*cur != ':'))
                        {
                            while ((cur < end) && (*cur != ';'))
                            {
                                ++cur;
                            }
                        }
                        else
                        {
                            ++cur;
                            const char* valueBegin = cur;
                            while ((cur < end) && (*cur != ';'))
                            {
                                ++cur;
                            }

                            const StringView value{ valueBegin, static_cast<uint32_t>(cur - valueBegin) };
                            ApplyStyleProperty(state.style, name, value);
                        }

                        if ((cur < end) && (*cur == ';'))
                        {
                            ++cur;
                        }
                    }
                }
                else
                {
                    ApplyStyleProperty(state.style, attr.name, attr.value);
                }

                if (attr.name.EqualToI("clip-path"))
                {
                    StringView trimmedValue = TrimView(attr.value);
                    const StringView prefix("url(#");
                    if ((trimmedValue.Size() > prefix.Size())
                        && trimmedValue.StartsWith(prefix)
                        && (trimmedValue[trimmedValue.Size() - 1] == ')'))
                    {
                        state.clipPathRef = trimmedValue.Substring(prefix.Size(), trimmedValue.Size() - prefix.Size() - 1);
                    }
                }
            }
        }

        class SvgParser final
        {
        public:
            bool Parse(ParsedImage& out, StringView text, float flatteningTolerance);

        private:
            bool ParseContainer(ParsedImage& out, const ParseState& inherited, StringView closingTag);
            void SkipWhitespace();
            void SkipTrivia();
            bool SkipMarkup();
            bool Consume(char c);
            StringView ParseName();
            bool ParseAttributes(Vector<Attribute>& out, bool& outSelfClosing);
            void ParseSvgAttributes(ParsedImage& out, Span<const Attribute> attrs);
            bool ParsePathElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParseRectElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParseCircleElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParseEllipseElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParseLineElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParsePolylineElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParsePolygonElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParseTextElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs, bool selfClosing);
            bool ParseUseElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParsePathData(
                CurveBuilder& builder,
                Vector<StrokeSourcePoint>& outPoints,
                Vector<StrokeSourceCommand>& outCommands,
                const Affine2D& strokeTransform,
                StringView text);
            bool ReadTextContents(String& outText, StringView closingTag);
            bool EmitParsedShape(
                ParsedImage& out,
                const ParseState& state,
                StringView id,
                Vector<CurveData>& curves,
                Vector<StrokeSourcePoint>& strokePoints,
                Vector<StrokeSourceCommand>& strokeCommands);
            bool SkipToClosingTag(StringView tagName);
            bool SkipElement(StringView tagName);
            const ParsedShape* FindDefinition(StringView id) const;
            const ParsedClipPath* FindClipPath(StringView id) const;

        private:
            StringView m_text{};
            const char* m_cur{ nullptr };
            const char* m_end{ nullptr };
            float m_flatteningTolerance{ 0.25f };
            Vector<ParsedDefinition> m_definitions{};
            Vector<ParsedClipPath> m_clipPaths{};
        };

        bool SvgParser::Parse(ParsedImage& out, StringView text, float flatteningTolerance)
        {
            m_text = text;
            m_cur = text.Data();
            m_end = text.Data() + text.Size();
            m_flatteningTolerance = Max(flatteningTolerance, 0.01f);
            out = {};
            m_definitions.Clear();
            m_clipPaths.Clear();

            ParseState rootState{};
            if (!ParseContainer(out, rootState, {}))
            {
                return false;
            }

            if (out.shapes.IsEmpty() && out.textRuns.IsEmpty())
            {
                HE_LOG_ERROR(he_scribe, HE_MSG("SVG parser did not find any filled path geometry."));
                return false;
            }

            if (!out.hasViewBox)
            {
                out.viewBoxMinX = out.boundsMinX;
                out.viewBoxMinY = out.boundsMinY;
                out.viewBoxWidth = Max(out.boundsMaxX - out.boundsMinX, 1.0f);
                out.viewBoxHeight = Max(out.boundsMaxY - out.boundsMinY, 1.0f);
                out.hasViewBox = true;
            }

            return true;
        }

        bool SvgParser::ParseContainer(ParsedImage& out, const ParseState& inherited, StringView closingTag)
        {
            while (true)
            {
                SkipTrivia();
                if (m_cur >= m_end)
                {
                    return closingTag.IsEmpty();
                }

                if ((m_end - m_cur) >= 2 && (m_cur[0] == '<') && (m_cur[1] == '/'))
                {
                    m_cur += 2;
                    const StringView tagName = ParseName();
                    if (tagName.IsEmpty())
                    {
                        return false;
                    }

                    SkipWhitespace();
                    if (!Consume('>'))
                    {
                        return false;
                    }

                    return tagName.EqualToI(closingTag);
                }

                if (!Consume('<'))
                {
                    return false;
                }

                if ((m_cur < m_end) && ((*m_cur == '!') || (*m_cur == '?')))
                {
                    if (!SkipMarkup())
                    {
                        return false;
                    }
                    continue;
                }

                const StringView tagName = ParseName();
                if (tagName.IsEmpty())
                {
                    return false;
                }

                Vector<Attribute> attrs{};
                bool selfClosing = false;
                if (!ParseAttributes(attrs, selfClosing))
                {
                    return false;
                }

                ParseState state = inherited;
                ApplyPresentationAttributes(state, attrs);

                if (tagName.EqualToI("svg"))
                {
                    ParseSvgAttributes(out, attrs);
                }
                else if (tagName.EqualToI("defs"))
                {
                    state.inDefinitions = true;
                    state.suppressOutput = true;
                }
                else if (tagName.EqualToI("clipPath"))
                {
                    state.inClipPath = true;
                    state.suppressOutput = true;
                    for (const Attribute& attr : attrs)
                    {
                        if (attr.name.EqualToI("id"))
                        {
                            state.activeClipPathId = attr.value;
                            break;
                        }
                    }
                }
                else if (tagName.EqualToI("mask")
                    || tagName.EqualToI("filter")
                    || tagName.EqualToI("symbol"))
                {
                    state.suppressOutput = true;
                }

                if (tagName.EqualToI("path"))
                {
                    if (!ParsePathElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("rect"))
                {
                    if (!ParseRectElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("circle"))
                {
                    if (!ParseCircleElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("ellipse"))
                {
                    if (!ParseEllipseElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("line"))
                {
                    if (!ParseLineElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("polyline"))
                {
                    if (!ParsePolylineElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("polygon"))
                {
                    if (!ParsePolygonElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("use"))
                {
                    if (!ParseUseElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("text"))
                {
                    if (!ParseTextElement(out, state, attrs, selfClosing))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("tspan")
                    || tagName.EqualToI("title")
                    || tagName.EqualToI("desc")
                    || tagName.EqualToI("metadata")
                    || tagName.EqualToI("style")
                    || tagName.EqualToI("script"))
                {
                    if (!selfClosing && !SkipElement(tagName))
                    {
                        return false;
                    }
                }
                else if (!selfClosing)
                {
                    if (!ParseContainer(out, state, tagName))
                    {
                        return false;
                    }
                }
            }
        }

        void SvgParser::SkipWhitespace()
        {
            while ((m_cur < m_end) && IsWhitespace(*m_cur))
            {
                ++m_cur;
            }
        }

        void SvgParser::SkipTrivia()
        {
            while (true)
            {
                SkipWhitespace();
                if (((m_end - m_cur) >= 4)
                    && (m_cur[0] == '<')
                    && (m_cur[1] == '!')
                    && (m_cur[2] == '-')
                    && (m_cur[3] == '-'))
                {
                    m_cur += 4;
                    while (((m_end - m_cur) >= 3)
                        && !((m_cur[0] == '-') && (m_cur[1] == '-') && (m_cur[2] == '>')))
                    {
                        ++m_cur;
                    }

                    if ((m_end - m_cur) < 3)
                    {
                        return;
                    }

                    m_cur += 3;
                    continue;
                }

                break;
            }
        }

        bool SvgParser::SkipMarkup()
        {
            while ((m_cur < m_end) && (*m_cur != '>'))
            {
                ++m_cur;
            }

            return Consume('>');
        }

        bool SvgParser::Consume(char c)
        {
            if ((m_cur < m_end) && (*m_cur == c))
            {
                ++m_cur;
                return true;
            }

            return false;
        }

        StringView SvgParser::ParseName()
        {
            const char* begin = m_cur;
            while ((m_cur < m_end) && IsNameChar(*m_cur))
            {
                ++m_cur;
            }

            return { begin, static_cast<uint32_t>(m_cur - begin) };
        }

        bool SvgParser::ParseAttributes(Vector<Attribute>& out, bool& outSelfClosing)
        {
            out.Clear();
            outSelfClosing = false;

            while (true)
            {
                SkipWhitespace();
                if (m_cur >= m_end)
                {
                    return false;
                }

                if (*m_cur == '/')
                {
                    ++m_cur;
                    outSelfClosing = true;
                    return Consume('>');
                }

                if (*m_cur == '>')
                {
                    ++m_cur;
                    return true;
                }

                const StringView name = ParseName();
                if (name.IsEmpty())
                {
                    return false;
                }

                SkipWhitespace();
                if (!Consume('='))
                {
                    return false;
                }

                SkipWhitespace();
                if ((m_cur >= m_end) || ((*m_cur != '"') && (*m_cur != '\'')))
                {
                    return false;
                }

                const char quote = *m_cur++;
                const char* valueBegin = m_cur;
                while ((m_cur < m_end) && (*m_cur != quote))
                {
                    ++m_cur;
                }

                if (m_cur >= m_end)
                {
                    return false;
                }

                const StringView value{ valueBegin, static_cast<uint32_t>(m_cur - valueBegin) };
                ++m_cur;

                Attribute& attr = out.EmplaceBack();
                attr.name = name;
                attr.value = value;
            }
        }

        void SvgParser::ParseSvgAttributes(ParsedImage& out, Span<const Attribute> attrs)
        {
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("viewBox"))
                {
                    Vector<float> values{};
                    if (ParseNumberList(values, attr.value) && (values.Size() == 4))
                    {
                        out.viewBoxMinX = values[0];
                        out.viewBoxMinY = values[1];
                        out.viewBoxWidth = Max(values[2], 1.0f);
                        out.viewBoxHeight = Max(values[3], 1.0f);
                        out.hasViewBox = true;
                    }
                }
                else if (!out.hasViewBox && attr.name.EqualToI("width"))
                {
                    ParseFloat(attr.value, out.viewBoxWidth);
                }
                else if (!out.hasViewBox && attr.name.EqualToI("height"))
                {
                    ParseFloat(attr.value, out.viewBoxHeight);
                }
            }
        }

        bool SvgParser::EmitParsedShape(
            ParsedImage& out,
            const ParseState& state,
            StringView id,
            Vector<CurveData>& curves,
            Vector<StrokeSourcePoint>& strokePoints,
            Vector<StrokeSourceCommand>& strokeCommands)
        {
            if (curves.IsEmpty() && strokeCommands.IsEmpty())
            {
                return true;
            }

            ParsedShape shape{};
            shape.curves = Move(curves);
            shape.strokePoints = Move(strokePoints);
            shape.strokeCommands = Move(strokeCommands);
            shape.fillRule = state.style.fillRule;
            shape.color = state.style.fill;
            RecomputeShapeBounds(shape);

            if (state.inClipPath)
            {
                if (!state.activeClipPathId.IsEmpty())
                {
                    ParsedClipPath* clipPath = nullptr;
                    for (ParsedClipPath& existing : m_clipPaths)
                    {
                        if (StringView(existing.id.Data(), existing.id.Size()).EqualToI(state.activeClipPathId))
                        {
                            clipPath = &existing;
                            break;
                        }
                    }

                    if (clipPath == nullptr)
                    {
                        clipPath = &m_clipPaths.EmplaceBack();
                        clipPath->id = String(state.activeClipPathId);
                        clipPath->minX = shape.minX;
                        clipPath->minY = shape.minY;
                        clipPath->maxX = shape.maxX;
                        clipPath->maxY = shape.maxY;
                    }
                    else
                    {
                        clipPath->minX = Min(clipPath->minX, shape.minX);
                        clipPath->minY = Min(clipPath->minY, shape.minY);
                        clipPath->maxX = Max(clipPath->maxX, shape.maxX);
                        clipPath->maxY = Max(clipPath->maxY, shape.maxY);
                    }
                }

                return true;
            }

            if (state.inDefinitions)
            {
                if (!id.IsEmpty())
                {
                    ParsedDefinition& definition = m_definitions.EmplaceBack();
                    definition.id = String(id);
                    definition.shape = Move(shape);
                }

                return true;
            }

            const StyleState effectiveStrokeStyle = BuildEffectiveStrokeStyle(state.style, state.transform);
            const float strokeMetricScale = ComputeAffineStrokeMetricScale(state.transform);
            const bool hasVisibleFill = !state.style.fillNone && (state.style.fill.w > 0.0f) && !shape.curves.IsEmpty();
            const bool hasVisibleStroke =
                !state.style.strokeNone
                && (state.style.stroke.w > 0.0f)
                && (effectiveStrokeStyle.strokeWidth > 0.0f)
                && !shape.strokeCommands.IsEmpty();
            if (state.suppressOutput || (!hasVisibleFill && !hasVisibleStroke))
            {
                return true;
            }

            if (!state.clipPathRef.IsEmpty())
            {
                const ParsedClipPath* clipPath = FindClipPath(state.clipPathRef);
                if (clipPath != nullptr)
                {
                    const bool overlaps = (shape.maxX > clipPath->minX)
                        && (shape.maxY > clipPath->minY)
                        && (shape.minX < clipPath->maxX)
                        && (shape.minY < clipPath->maxY);
                    if (!overlaps)
                    {
                        return true;
                    }
                }
            }

            uint32_t fillShapeIndex = 0;
            if (hasVisibleFill)
            {
                fillShapeIndex = AppendParsedShape(out, Move(shape));
            }

            if (hasVisibleFill)
            {
                CompiledVectorImageLayerEntry& layer = out.layers.EmplaceBack();
                layer.shapeIndex = fillShapeIndex;
                layer.kind = VectorLayerKind::Fill;
                layer.red = state.style.fill.x;
                layer.green = state.style.fill.y;
                layer.blue = state.style.fill.z;
                layer.alpha = state.style.fill.w;
            }

            if (hasVisibleStroke)
            {
                const ParsedShape& strokeSource = hasVisibleFill ? out.shapes[fillShapeIndex] : shape;
                const bool splitDashedLineStrokes =
                    !effectiveStrokeStyle.strokeDashArray.IsEmpty()
                    && IsLineOnlyStrokeSource(strokeSource);

                if (splitDashedLineStrokes)
                {
                    Vector<ParsedShape> strokeShapes{};
                    if (!BuildAuthoredDashedLineStrokeShapes(
                            strokeShapes,
                            strokeSource,
                            effectiveStrokeStyle,
                            strokeMetricScale,
                            m_flatteningTolerance))
                    {
                        return false;
                    }

                    for (uint32_t strokeShapeIndex = 0; strokeShapeIndex < strokeShapes.Size(); ++strokeShapeIndex)
                    {
                        const uint32_t appendedShapeIndex = AppendParsedShape(out, Move(strokeShapes[strokeShapeIndex]));
                        CompiledVectorImageLayerEntry& layer = out.layers.EmplaceBack();
                        layer.shapeIndex = appendedShapeIndex;
                        layer.kind = VectorLayerKind::Stroke;
                        layer.red = state.style.stroke.x;
                        layer.green = state.style.stroke.y;
                        layer.blue = state.style.stroke.z;
                        layer.alpha = state.style.stroke.w;
                        layer.strokeWidth = effectiveStrokeStyle.strokeWidth;
                        layer.strokeJoin = effectiveStrokeStyle.strokeJoin;
                        layer.strokeCap = effectiveStrokeStyle.strokeCap;
                        layer.strokeMiterLimit = effectiveStrokeStyle.strokeMiterLimit;
                    }
                }
                else
                {
                    ParsedShape strokeShape{};
                    if (!BuildAuthoredStrokeShape(
                            strokeShape,
                            strokeSource,
                            effectiveStrokeStyle,
                            strokeMetricScale,
                            m_flatteningTolerance))
                    {
                        return false;
                    }

                    const uint32_t strokeShapeIndex = AppendParsedShape(out, Move(strokeShape));
                    CompiledVectorImageLayerEntry& layer = out.layers.EmplaceBack();
                    layer.shapeIndex = strokeShapeIndex;
                    layer.kind = VectorLayerKind::Stroke;
                    layer.red = state.style.stroke.x;
                    layer.green = state.style.stroke.y;
                    layer.blue = state.style.stroke.z;
                    layer.alpha = state.style.stroke.w;
                    layer.strokeWidth = effectiveStrokeStyle.strokeWidth;
                    layer.strokeJoin = effectiveStrokeStyle.strokeJoin;
                    layer.strokeCap = effectiveStrokeStyle.strokeCap;
                    layer.strokeMiterLimit = effectiveStrokeStyle.strokeMiterLimit;
                }
            }

            return true;
        }

        bool SvgParser::ParsePathElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            StringView d{};
            StringView id{};
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("d"))
                {
                    d = attr.value;
                }
                else if (attr.name.EqualToI("id"))
                {
                    id = attr.value;
                }
            }

            if (d.IsEmpty())
            {
                return true;
            }

            CurveBuilder builder(m_flatteningTolerance);
            builder.SetTransform(state.transform);
            Vector<StrokeSourcePoint> strokePoints{};
            Vector<StrokeSourceCommand> strokeCommands{};
            if (!ParsePathData(builder, strokePoints, strokeCommands, state.transform, d))
            {
                HE_LOG_ERROR(he_scribe, HE_MSG("Failed to parse SVG path data for scribe image compile."));
                return false;
            }

            return EmitParsedShape(out, state, id, builder.Curves(), strokePoints, strokeCommands);
        }

        bool SvgParser::ParseRectElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            float x = 0.0f;
            float y = 0.0f;
            float width = 0.0f;
            float height = 0.0f;
            float rx = 0.0f;
            float ry = 0.0f;
            StringView id{};

            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("x")) { ParseFloat(attr.value, x); }
                else if (attr.name.EqualToI("y")) { ParseFloat(attr.value, y); }
                else if (attr.name.EqualToI("width")) { ParseFloat(attr.value, width); }
                else if (attr.name.EqualToI("height")) { ParseFloat(attr.value, height); }
                else if (attr.name.EqualToI("rx")) { ParseFloat(attr.value, rx); }
                else if (attr.name.EqualToI("ry")) { ParseFloat(attr.value, ry); }
                else if (attr.name.EqualToI("id")) { id = attr.value; }
            }

            if ((width <= 0.0f) || (height <= 0.0f))
            {
                return true;
            }

            if ((rx > 0.0f) && (ry <= 0.0f))
            {
                ry = rx;
            }
            else if ((ry > 0.0f) && (rx <= 0.0f))
            {
                rx = ry;
            }

            rx = Clamp(rx, 0.0f, width * 0.5f);
            ry = Clamp(ry, 0.0f, height * 0.5f);

            constexpr float Kappa = 0.5522847498307936f;
            auto tx = [&](float px, float py) -> Point2
            {
                return TransformPoint(state.transform, { px, py });
            };

            CurveBuilder builder(m_flatteningTolerance);
            StrokeSourceBuilder strokeBuilder(m_flatteningTolerance);
            if ((rx <= 0.0f) || (ry <= 0.0f))
            {
                const Point2 p0 = tx(x, y);
                const Point2 p1 = tx(x + width, y);
                const Point2 p2 = tx(x + width, y + height);
                const Point2 p3 = tx(x, y + height);

                strokeBuilder.AppendMoveTo(p0);
                builder.AddLine(p0, p1);
                strokeBuilder.AppendLineTo(p1);
                builder.AddLine(p1, p2);
                strokeBuilder.AppendLineTo(p2);
                builder.AddLine(p2, p3);
                strokeBuilder.AppendLineTo(p3);
                builder.AddLine(p3, p0);
                strokeBuilder.AppendClose();
            }
            else
            {
                const float cx0 = x + rx;
                const float cx1 = x + width - rx;
                const float cy0 = y + ry;
                const float cy1 = y + height - ry;
                const float ox = rx * Kappa;
                const float oy = ry * Kappa;

                const Point2 start = tx(cx0, y);
                strokeBuilder.AppendMoveTo(start);

                const Point2 topRight = tx(cx1, y);
                builder.AddLine(start, topRight);
                strokeBuilder.AppendLineTo(topRight);

                const Point2 trc1 = tx(cx1 + ox, y);
                const Point2 trc2 = tx(x + width, cy0 - oy);
                const Point2 tr = tx(x + width, cy0);
                builder.AddCubic(topRight, trc1, trc2, tr);
                strokeBuilder.AppendCubicTo(trc1, trc2, tr);

                const Point2 bottomRight = tx(x + width, cy1);
                builder.AddLine(tr, bottomRight);
                strokeBuilder.AppendLineTo(bottomRight);

                const Point2 brc1 = tx(x + width, cy1 + oy);
                const Point2 brc2 = tx(cx1 + ox, y + height);
                const Point2 br = tx(cx1, y + height);
                builder.AddCubic(bottomRight, brc1, brc2, br);
                strokeBuilder.AppendCubicTo(brc1, brc2, br);

                const Point2 bottomLeft = tx(cx0, y + height);
                builder.AddLine(br, bottomLeft);
                strokeBuilder.AppendLineTo(bottomLeft);

                const Point2 blc1 = tx(cx0 - ox, y + height);
                const Point2 blc2 = tx(x, cy1 + oy);
                const Point2 bl = tx(x, cy1);
                builder.AddCubic(bottomLeft, blc1, blc2, bl);
                strokeBuilder.AppendCubicTo(blc1, blc2, bl);

                const Point2 topLeft = tx(x, cy0);
                builder.AddLine(bl, topLeft);
                strokeBuilder.AppendLineTo(topLeft);

                const Point2 tlc1 = tx(x, cy0 - oy);
                const Point2 tlc2 = tx(cx0 - ox, y);
                builder.AddCubic(topLeft, tlc1, tlc2, start);
                strokeBuilder.AppendCubicTo(tlc1, tlc2, start);
                strokeBuilder.AppendClose();
            }

            Vector<StrokeSourcePoint> strokePoints = Move(strokeBuilder.Points());
            Vector<StrokeSourceCommand> strokeCommands = Move(strokeBuilder.Commands());
            return EmitParsedShape(out, state, id, builder.Curves(), strokePoints, strokeCommands);
        }

        bool SvgParser::ParseCircleElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            float cx = 0.0f;
            float cy = 0.0f;
            float r = 0.0f;
            StringView id{};

            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("cx")) { ParseFloat(attr.value, cx); }
                else if (attr.name.EqualToI("cy")) { ParseFloat(attr.value, cy); }
                else if (attr.name.EqualToI("r")) { ParseFloat(attr.value, r); }
                else if (attr.name.EqualToI("id")) { id = attr.value; }
            }

            if (r <= 0.0f)
            {
                return true;
            }

            auto tx = [&](float px, float py) -> Point2
            {
                return TransformPoint(state.transform, { px, py });
            };

            constexpr float Kappa = 0.5522847498307936f;
            CurveBuilder builder(m_flatteningTolerance);
            StrokeSourceBuilder strokeBuilder(m_flatteningTolerance);
            const Point2 start = tx(cx + r, cy);
            strokeBuilder.AppendMoveTo(start);

            const Point2 c1 = tx(cx + r, cy + (r * Kappa));
            const Point2 c2 = tx(cx + (r * Kappa), cy + r);
            const Point2 p1 = tx(cx, cy + r);
            builder.AddCubic(start, c1, c2, p1);
            strokeBuilder.AppendCubicTo(c1, c2, p1);

            const Point2 c3 = tx(cx - (r * Kappa), cy + r);
            const Point2 c4 = tx(cx - r, cy + (r * Kappa));
            const Point2 p2 = tx(cx - r, cy);
            builder.AddCubic(p1, c3, c4, p2);
            strokeBuilder.AppendCubicTo(c3, c4, p2);

            const Point2 c5 = tx(cx - r, cy - (r * Kappa));
            const Point2 c6 = tx(cx - (r * Kappa), cy - r);
            const Point2 p3 = tx(cx, cy - r);
            builder.AddCubic(p2, c5, c6, p3);
            strokeBuilder.AppendCubicTo(c5, c6, p3);

            const Point2 c7 = tx(cx + (r * Kappa), cy - r);
            const Point2 c8 = tx(cx + r, cy - (r * Kappa));
            builder.AddCubic(p3, c7, c8, start);
            strokeBuilder.AppendCubicTo(c7, c8, start);
            strokeBuilder.AppendClose();

            Vector<StrokeSourcePoint> strokePoints = Move(strokeBuilder.Points());
            Vector<StrokeSourceCommand> strokeCommands = Move(strokeBuilder.Commands());
            return EmitParsedShape(out, state, id, builder.Curves(), strokePoints, strokeCommands);
        }

        bool SvgParser::ParseEllipseElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            float cx = 0.0f;
            float cy = 0.0f;
            float rx = 0.0f;
            float ry = 0.0f;
            StringView id{};

            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("cx")) { ParseFloat(attr.value, cx); }
                else if (attr.name.EqualToI("cy")) { ParseFloat(attr.value, cy); }
                else if (attr.name.EqualToI("rx")) { ParseFloat(attr.value, rx); }
                else if (attr.name.EqualToI("ry")) { ParseFloat(attr.value, ry); }
                else if (attr.name.EqualToI("id")) { id = attr.value; }
            }

            if ((rx <= 0.0f) || (ry <= 0.0f))
            {
                return true;
            }

            auto tx = [&](float px, float py) -> Point2
            {
                return TransformPoint(state.transform, { px, py });
            };

            constexpr float Kappa = 0.5522847498307936f;
            CurveBuilder builder(m_flatteningTolerance);
            StrokeSourceBuilder strokeBuilder(m_flatteningTolerance);
            const Point2 start = tx(cx + rx, cy);
            strokeBuilder.AppendMoveTo(start);

            const Point2 c1 = tx(cx + rx, cy + (ry * Kappa));
            const Point2 c2 = tx(cx + (rx * Kappa), cy + ry);
            const Point2 p1 = tx(cx, cy + ry);
            builder.AddCubic(start, c1, c2, p1);
            strokeBuilder.AppendCubicTo(c1, c2, p1);

            const Point2 c3 = tx(cx - (rx * Kappa), cy + ry);
            const Point2 c4 = tx(cx - rx, cy + (ry * Kappa));
            const Point2 p2 = tx(cx - rx, cy);
            builder.AddCubic(p1, c3, c4, p2);
            strokeBuilder.AppendCubicTo(c3, c4, p2);

            const Point2 c5 = tx(cx - rx, cy - (ry * Kappa));
            const Point2 c6 = tx(cx - (rx * Kappa), cy - ry);
            const Point2 p3 = tx(cx, cy - ry);
            builder.AddCubic(p2, c5, c6, p3);
            strokeBuilder.AppendCubicTo(c5, c6, p3);

            const Point2 c7 = tx(cx + (rx * Kappa), cy - ry);
            const Point2 c8 = tx(cx + rx, cy - (ry * Kappa));
            builder.AddCubic(p3, c7, c8, start);
            strokeBuilder.AppendCubicTo(c7, c8, start);
            strokeBuilder.AppendClose();

            Vector<StrokeSourcePoint> strokePoints = Move(strokeBuilder.Points());
            Vector<StrokeSourceCommand> strokeCommands = Move(strokeBuilder.Commands());
            return EmitParsedShape(out, state, id, builder.Curves(), strokePoints, strokeCommands);
        }

        bool SvgParser::ParseLineElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            float x1 = 0.0f;
            float y1 = 0.0f;
            float x2 = 0.0f;
            float y2 = 0.0f;
            StringView id{};

            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("x1")) { ParseFloat(attr.value, x1); }
                else if (attr.name.EqualToI("y1")) { ParseFloat(attr.value, y1); }
                else if (attr.name.EqualToI("x2")) { ParseFloat(attr.value, x2); }
                else if (attr.name.EqualToI("y2")) { ParseFloat(attr.value, y2); }
                else if (attr.name.EqualToI("id")) { id = attr.value; }
            }

            const Point2 p0 = TransformPoint(state.transform, { x1, y1 });
            const Point2 p1 = TransformPoint(state.transform, { x2, y2 });

            CurveBuilder builder(m_flatteningTolerance);
            builder.AddLine(p0, p1);

            StrokeSourceBuilder strokeBuilder(m_flatteningTolerance);
            strokeBuilder.AppendMoveTo(p0);
            strokeBuilder.AppendLineTo(p1);

            Vector<StrokeSourcePoint> strokePoints = Move(strokeBuilder.Points());
            Vector<StrokeSourceCommand> strokeCommands = Move(strokeBuilder.Commands());
            ParseState lineState = state;
            lineState.style.fillNone = true;
            lineState.style.fill = { 0.0f, 0.0f, 0.0f, 0.0f };
            return EmitParsedShape(out, lineState, id, builder.Curves(), strokePoints, strokeCommands);
        }

        bool SvgParser::ParsePolylineElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            StringView pointsAttr{};
            StringView id{};
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("points")) { pointsAttr = attr.value; }
                else if (attr.name.EqualToI("id")) { id = attr.value; }
            }

            if (pointsAttr.IsEmpty())
            {
                return true;
            }

            Vector<float> values{};
            if (!ParseNumberList(values, pointsAttr) || (values.Size() < 4) || ((values.Size() & 1u) != 0u))
            {
                return false;
            }

            CurveBuilder builder(m_flatteningTolerance);
            StrokeSourceBuilder strokeBuilder(m_flatteningTolerance);
            Point2 first = TransformPoint(state.transform, { values[0], values[1] });
            Point2 previous = first;
            strokeBuilder.AppendMoveTo(first);
            for (uint32_t pointIndex = 2; pointIndex < values.Size(); pointIndex += 2)
            {
                const Point2 current = TransformPoint(state.transform, { values[pointIndex], values[pointIndex + 1] });
                builder.AddLine(previous, current);
                strokeBuilder.AppendLineTo(current);
                previous = current;
            }

            if (!state.style.fillNone && (state.style.fill.w > 0.0f))
            {
                builder.AddLine(previous, first);
            }

            Vector<StrokeSourcePoint> strokePoints = Move(strokeBuilder.Points());
            Vector<StrokeSourceCommand> strokeCommands = Move(strokeBuilder.Commands());
            return EmitParsedShape(out, state, id, builder.Curves(), strokePoints, strokeCommands);
        }

        bool SvgParser::ParsePolygonElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            StringView pointsAttr{};
            StringView id{};
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("points")) { pointsAttr = attr.value; }
                else if (attr.name.EqualToI("id")) { id = attr.value; }
            }

            if (pointsAttr.IsEmpty())
            {
                return true;
            }

            Vector<float> values{};
            if (!ParseNumberList(values, pointsAttr) || (values.Size() < 6) || ((values.Size() & 1u) != 0u))
            {
                return false;
            }

            CurveBuilder builder(m_flatteningTolerance);
            StrokeSourceBuilder strokeBuilder(m_flatteningTolerance);
            Point2 first = TransformPoint(state.transform, { values[0], values[1] });
            Point2 previous = first;
            strokeBuilder.AppendMoveTo(first);
            for (uint32_t pointIndex = 2; pointIndex < values.Size(); pointIndex += 2)
            {
                const Point2 current = TransformPoint(state.transform, { values[pointIndex], values[pointIndex + 1] });
                builder.AddLine(previous, current);
                strokeBuilder.AppendLineTo(current);
                previous = current;
            }

            builder.AddLine(previous, first);
            strokeBuilder.AppendClose();

            Vector<StrokeSourcePoint> strokePoints = Move(strokeBuilder.Points());
            Vector<StrokeSourceCommand> strokeCommands = Move(strokeBuilder.Commands());
            return EmitParsedShape(out, state, id, builder.Curves(), strokePoints, strokeCommands);
        }

        bool SvgParser::ReadTextContents(String& outText, StringView closingTag)
        {
            while (m_cur < m_end)
            {
                if ((m_end - m_cur) >= 2 && (m_cur[0] == '<') && (m_cur[1] == '/'))
                {
                    m_cur += 2;
                    const StringView tagName = ParseName();
                    if (tagName.IsEmpty())
                    {
                        return false;
                    }

                    SkipWhitespace();
                    if (!Consume('>'))
                    {
                        return false;
                    }

                    return tagName.EqualToI(closingTag);
                }

                if (*m_cur != '<')
                {
                    const char* textBegin = m_cur;
                    while ((m_cur < m_end) && (*m_cur != '<'))
                    {
                        ++m_cur;
                    }

                    if (m_cur > textBegin)
                    {
                        AppendDecodedSvgText(outText, StringView(textBegin, static_cast<uint32_t>(m_cur - textBegin)));
                    }
                    continue;
                }

                if (!Consume('<'))
                {
                    return false;
                }

                if ((m_cur < m_end) && ((*m_cur == '!') || (*m_cur == '?')))
                {
                    if (!SkipMarkup())
                    {
                        return false;
                    }
                    continue;
                }

                const bool isClosing = Consume('/');
                const StringView tagName = ParseName();
                if (tagName.IsEmpty())
                {
                    return false;
                }

                Vector<Attribute> attrs{};
                bool selfClosing = false;
                if (!ParseAttributes(attrs, selfClosing))
                {
                    return false;
                }

                if (isClosing)
                {
                    if (tagName.EqualToI(closingTag))
                    {
                        return true;
                    }

                    return false;
                }

                if (!selfClosing && tagName.EqualToI("tspan"))
                {
                    if (!ReadTextContents(outText, tagName))
                    {
                        return false;
                    }
                }
                else if (!selfClosing && !SkipElement(tagName))
                {
                    return false;
                }
            }

            return closingTag.IsEmpty();
        }

        bool SvgParser::ParseTextElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs, bool selfClosing)
        {
            if (selfClosing)
            {
                return true;
            }

            float x = 0.0f;
            float y = 0.0f;
            float fontSize = 16.0f;
            StringView fontFamily("sans-serif");
            StringView fontStyle("normal");
            StringView fontWeight("normal");
            StringView textAnchor("start");
            StringView id{};
            Vector<float> textXPositions{};
            Vector<float> textYPositions{};
            Vector<float> textDxPositions{};
            Vector<float> textDyPositions{};
            bool preserveWhitespace = ShouldPreserveSvgTextWhitespace(attrs, false);
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("x"))
                {
                    if (ParseNumberList(textXPositions, attr.value) && !textXPositions.IsEmpty())
                    {
                        x = textXPositions[0];
                    }
                }
                else if (attr.name.EqualToI("y"))
                {
                    if (ParseNumberList(textYPositions, attr.value) && !textYPositions.IsEmpty())
                    {
                        y = textYPositions[0];
                    }
                }
                else if (attr.name.EqualToI("dx"))
                {
                    ParseNumberList(textDxPositions, attr.value);
                }
                else if (attr.name.EqualToI("dy"))
                {
                    ParseNumberList(textDyPositions, attr.value);
                }
                else if (attr.name.EqualToI("font-size")) { ParseFloat(attr.value, fontSize); }
                else if (attr.name.EqualToI("font-family")) { fontFamily = attr.value; }
                else if (attr.name.EqualToI("font-style")) { fontStyle = attr.value; }
                else if (attr.name.EqualToI("font-weight")) { fontWeight = attr.value; }
                else if (attr.name.EqualToI("text-anchor")) { textAnchor = TrimView(attr.value); }
                else if (attr.name.EqualToI("id")) { id = attr.value; }
            }

            struct TextSpan
            {
                String text{};
                float x{ 0.0f };
                float y{ 0.0f };
                float fontSize{ 16.0f };
                String fontKey{};
                Vector<float> xPositions{};
                Vector<float> yPositions{};
                Vector<float> dxPositions{};
                Vector<float> dyPositions{};
            };

            Vector<TextSpan> spans{};
            auto appendSpan = [&spans](String&& decodedText,
                float spanX,
                float spanY,
                float spanFontSize,
                StringView spanFontFamily,
                StringView spanFontStyle,
                StringView spanFontWeight,
                const Vector<float>& spanXPositions,
                const Vector<float>& spanYPositions,
                const Vector<float>& spanDxPositions,
                const Vector<float>& spanDyPositions,
                bool preserveSpace) -> void
            {
                if (preserveSpace)
                {
                    if (decodedText.IsEmpty())
                    {
                        return;
                    }

                    TextSpan& span = spans.EmplaceBack();
                    span.text = Move(decodedText);
                    span.x = spanX;
                    span.y = spanY;
                    span.fontSize = spanFontSize;
                    span.fontKey = BuildSvgFontKey(spanFontFamily, spanFontStyle, spanFontWeight);
                    span.xPositions = spanXPositions;
                    span.yPositions = spanYPositions;
                    span.dxPositions = spanDxPositions;
                    span.dyPositions = spanDyPositions;
                    return;
                }

                const StringView trimmedText = UTF8Trim(StringView(decodedText.Data(), decodedText.Size()));
                if (trimmedText.IsEmpty())
                {
                    return;
                }

                TextSpan& span = spans.EmplaceBack();
                span.text = trimmedText;
                span.x = spanX;
                span.y = spanY;
                span.fontSize = spanFontSize;
                span.fontKey = BuildSvgFontKey(spanFontFamily, spanFontStyle, spanFontWeight);
                span.xPositions = spanXPositions;
                span.yPositions = spanYPositions;
                span.dxPositions = spanDxPositions;
                span.dyPositions = spanDyPositions;
            };

            while (m_cur < m_end)
            {
                if ((m_end - m_cur) >= 2 && (m_cur[0] == '<') && (m_cur[1] == '/'))
                {
                    m_cur += 2;
                    const StringView tagName = ParseName();
                    if (!tagName.EqualToI("text"))
                    {
                        return false;
                    }

                    SkipWhitespace();
                    if (!Consume('>'))
                    {
                        return false;
                    }

                    break;
                }

                if (*m_cur != '<')
                {
                    const char* begin = m_cur;
                    while ((m_cur < m_end) && (*m_cur != '<'))
                    {
                        ++m_cur;
                    }

                    String directText{};
                    AppendDecodedSvgText(directText, StringView(begin, static_cast<uint32_t>(m_cur - begin)));
                    appendSpan(
                        Move(directText),
                        x,
                        y,
                        fontSize,
                        fontFamily,
                        fontStyle,
                        fontWeight,
                        textXPositions,
                        textYPositions,
                        textDxPositions,
                        textDyPositions,
                        preserveWhitespace);
                    continue;
                }

                ++m_cur;
                if ((m_cur < m_end) && ((*m_cur == '!') || (*m_cur == '?')))
                {
                    if (!SkipMarkup())
                    {
                        return false;
                    }

                    continue;
                }

                const StringView tagName = ParseName();
                if (tagName.IsEmpty())
                {
                    return false;
                }

                Vector<Attribute> spanAttrs{};
                bool spanSelfClosing = false;
                if (!ParseAttributes(spanAttrs, spanSelfClosing))
                {
                    return false;
                }

                if (!tagName.EqualToI("tspan"))
                {
                    if (!spanSelfClosing && !SkipElement(tagName))
                    {
                        return false;
                    }
                    continue;
                }

                String spanText{};
                if (!spanSelfClosing && !ReadTextContents(spanText, "tspan"))
                {
                    return false;
                }

                float spanX = x;
                float spanY = y;
                float spanFontSize = fontSize;
                StringView spanFontFamily = fontFamily;
                StringView spanFontStyle = fontStyle;
                StringView spanFontWeight = fontWeight;
                Vector<float> spanXPositions = textXPositions;
                Vector<float> spanYPositions = textYPositions;
                Vector<float> spanDxPositions = textDxPositions;
                Vector<float> spanDyPositions = textDyPositions;
                const bool spanPreserveWhitespace = ShouldPreserveSvgTextWhitespace(spanAttrs, preserveWhitespace);
                for (const Attribute& spanAttr : spanAttrs)
                {
                    if (spanAttr.name.EqualToI("x"))
                    {
                        if (ParseNumberList(spanXPositions, spanAttr.value) && !spanXPositions.IsEmpty())
                        {
                            spanX = spanXPositions[0];
                        }
                    }
                    else if (spanAttr.name.EqualToI("y"))
                    {
                        if (ParseNumberList(spanYPositions, spanAttr.value) && !spanYPositions.IsEmpty())
                        {
                            spanY = spanYPositions[0];
                        }
                    }
                    else if (spanAttr.name.EqualToI("dx"))
                    {
                        ParseNumberList(spanDxPositions, spanAttr.value);
                    }
                    else if (spanAttr.name.EqualToI("dy"))
                    {
                        ParseNumberList(spanDyPositions, spanAttr.value);
                    }
                    else if (spanAttr.name.EqualToI("font-size"))
                    {
                        ParseFloat(spanAttr.value, spanFontSize);
                    }
                    else if (spanAttr.name.EqualToI("font-family"))
                    {
                        spanFontFamily = spanAttr.value;
                    }
                    else if (spanAttr.name.EqualToI("font-style"))
                    {
                        spanFontStyle = spanAttr.value;
                    }
                    else if (spanAttr.name.EqualToI("font-weight"))
                    {
                        spanFontWeight = spanAttr.value;
                    }
                }

                appendSpan(
                    Move(spanText),
                    spanX,
                    spanY,
                    spanFontSize,
                    spanFontFamily,
                    spanFontStyle,
                    spanFontWeight,
                    spanXPositions,
                    spanYPositions,
                    spanDxPositions,
                    spanDyPositions,
                    spanPreserveWhitespace);
            }

            const bool hasVisibleFill = !state.style.fillNone && (state.style.fill.w > 0.0f);
            const bool hasVisibleStroke = !state.style.strokeNone && (state.style.stroke.w > 0.0f) && (state.style.strokeWidth > 0.0f);
            if (!hasVisibleFill && !hasVisibleStroke)
            {
                return true;
            }

            ScribeImage::TextAnchorKind anchor = ScribeImage::TextAnchorKind::Start;
            if (textAnchor.EqualToI("middle"))
            {
                anchor = ScribeImage::TextAnchorKind::Middle;
            }
            else if (textAnchor.EqualToI("end"))
            {
                anchor = ScribeImage::TextAnchorKind::End;
            }

            auto appendCompiledTextRun = [&](uint32_t fontFaceIndex,
                ScribeImage::TextAnchorKind runAnchor,
                StringView runText,
                float runX,
                float runY,
                float runFontSize,
                bool positionUsesGlyphOriginX) -> void
            {
                CompiledVectorImageTextRunEntry& run = out.textRuns.EmplaceBack();
                run.fontFaceIndex = fontFaceIndex;
                run.anchor = runAnchor;
                run.text = runText;
                run.position = { runX, runY };
                run.fontSize = runFontSize;
                run.color = hasVisibleFill ? state.style.fill : Vec4f{ 0.0f, 0.0f, 0.0f, 0.0f };
                run.strokeColor = hasVisibleStroke ? state.style.stroke : Vec4f{ 0.0f, 0.0f, 0.0f, 0.0f };
                run.strokeWidth = hasVisibleStroke ? state.style.strokeWidth : 0.0f;
                run.strokeJoin = state.style.strokeJoin;
                run.strokeCap = state.style.strokeCap;
                run.strokeMiterLimit = state.style.strokeMiterLimit;
                run.positionUsesGlyphOriginX = positionUsesGlyphOriginX;
                ApplyAffineToTextRun(run, state.transform);
            };

            for (const TextSpan& span : spans)
            {
                const uint32_t fontFaceIndex = FindOrAppendSvgFontFace(out.fontFaces, span.fontKey);
                const uint32_t codePointCount = UTF8Length(span.text.Data(), span.text.Size());
                const bool hasExplicitGlyphX = !span.xPositions.IsEmpty() && ((span.xPositions.Size() == codePointCount) || (span.xPositions.Size() == 1u));
                const bool hasExplicitGlyphY = !span.yPositions.IsEmpty() && ((span.yPositions.Size() == codePointCount) || (span.yPositions.Size() == 1u));
                const bool hasExplicitGlyphDx = !span.dxPositions.IsEmpty() && (span.dxPositions.Size() == codePointCount);
                const bool hasExplicitGlyphDy = !span.dyPositions.IsEmpty() && (span.dyPositions.Size() == codePointCount);
                const bool useExplicitGlyphRuns =
                    (codePointCount > 0u)
                    && (hasExplicitGlyphX || hasExplicitGlyphY || hasExplicitGlyphDx || hasExplicitGlyphDy)
                    && (span.xPositions.IsEmpty() || hasExplicitGlyphX)
                    && (span.yPositions.IsEmpty() || hasExplicitGlyphY)
                    && (span.dxPositions.IsEmpty() || hasExplicitGlyphDx)
                    && (span.dyPositions.IsEmpty() || hasExplicitGlyphDy);
                if (!useExplicitGlyphRuns)
                {
                    appendCompiledTextRun(fontFaceIndex, anchor, StringView(span.text.Data(), span.text.Size()), span.x, span.y, span.fontSize, false);
                    continue;
                }

                const char* cur = span.text.Data();
                const char* end = span.text.End();
                uint32_t glyphIndex = 0;
                while (cur < end)
                {
                    uint32_t codePoint = InvalidCodePoint;
                    const uint32_t sequenceLength = UTF8Decode(codePoint, cur, static_cast<uint32_t>(end - cur));
                    if ((sequenceLength == 0) || (sequenceLength == InvalidCodePoint))
                    {
                        break;
                    }

                    float glyphX = span.x;
                    if (hasExplicitGlyphX)
                    {
                        glyphX = span.xPositions[(span.xPositions.Size() == 1u) ? 0u : glyphIndex];
                    }

                    float glyphY = span.y;
                    if (hasExplicitGlyphY)
                    {
                        glyphY = span.yPositions[(span.yPositions.Size() == 1u) ? 0u : glyphIndex];
                    }
                    if (hasExplicitGlyphDx)
                    {
                        glyphX += span.dxPositions[glyphIndex];
                    }
                    if (hasExplicitGlyphDy)
                    {
                        glyphY += span.dyPositions[glyphIndex];
                    }

                    appendCompiledTextRun(
                        fontFaceIndex,
                        ScribeImage::TextAnchorKind::Start,
                        StringView(cur, sequenceLength),
                        glyphX,
                        glyphY,
                        span.fontSize,
                        true);
                    cur += sequenceLength;
                    ++glyphIndex;
                }
            }

            return true;
        }

        bool SvgParser::ParseUseElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            const bool hasVisibleFill = !state.style.fillNone && (state.style.fill.w > 0.0f);
            const bool hasVisibleStroke = !state.style.strokeNone && (state.style.stroke.w > 0.0f) && (state.style.strokeWidth > 0.0f);
            if (state.suppressOutput || (!hasVisibleFill && !hasVisibleStroke))
            {
                return true;
            }

            const Attribute* hrefAttr = FindAttribute(attrs, "href");
            if (!hrefAttr)
            {
                hrefAttr = FindAttribute(attrs, "xlink:href");
            }

            if (!hrefAttr || hrefAttr->value.IsEmpty())
            {
                return true;
            }

            StringView refId = hrefAttr->value;
            if ((refId.Size() > 0) && (refId[0] == '#'))
            {
                refId = refId.Substring(1);
            }

            const ParsedShape* definition = FindDefinition(refId);
            if (!definition)
            {
                return true;
            }

            ParseState instanceState = state;

            float x = 0.0f;
            float y = 0.0f;
            if (const Attribute* xAttr = FindAttribute(attrs, "x"))
            {
                ParseFloat(xAttr->value, x);
            }

            if (const Attribute* yAttr = FindAttribute(attrs, "y"))
            {
                ParseFloat(yAttr->value, y);
            }

            if ((x != 0.0f) || (y != 0.0f))
            {
                Affine2D translation{};
                translation.tx = x;
                translation.ty = y;
                instanceState.transform = ComposeAffine(instanceState.transform, translation);
            }

            ParsedShape shape = CloneTransformedShape(*definition, instanceState);
            return EmitParsedShape(
                out,
                instanceState,
                {},
                shape.curves,
                shape.strokePoints,
                shape.strokeCommands);
        }

        bool SvgParser::ParsePathData(
            CurveBuilder& builder,
            Vector<StrokeSourcePoint>& outPoints,
            Vector<StrokeSourceCommand>& outCommands,
            const Affine2D& strokeTransform,
            StringView text)
        {
            const char* cur = text.Data();
            const char* end = text.Data() + text.Size();

            outPoints.Clear();
            outCommands.Clear();
            StrokeSourceBuilder strokeBuilder(m_flatteningTolerance);

            Point2 current{};
            Point2 subpathStart{};
            Point2 previousCubicControl{};
            Point2 previousQuadraticControl{};
            char command = 0;
            bool hasCurrent = false;
            bool subpathClosed = false;
            bool hasPreviousCubicControl = false;
            bool hasPreviousQuadraticControl = false;

            auto SkipSeparators = [&]()
            {
                while ((cur < end) && (IsWhitespace(*cur) || (*cur == ',')))
                {
                    ++cur;
                }
            };

            auto ReadFloatValue = [&](float& value) -> bool
            {
                SkipSeparators();
                if (cur >= end)
                {
                    return false;
                }

                const char* parseEnd = end;
                if (!StrToFloat(value, cur, &parseEnd) || (parseEnd == cur))
                {
                    return false;
                }

                cur = parseEnd;
                return true;
            };

            auto ReadPoint = [&](bool relative, Point2& point) -> bool
            {
                if (!ReadFloatValue(point.x) || !ReadFloatValue(point.y))
                {
                    return false;
                }

                if (relative)
                {
                    point.x += current.x;
                    point.y += current.y;
                }

                return true;
            };

            auto ToStrokePoint = [&](const Point2& point) -> Point2
            {
                return TransformPoint(strokeTransform, point);
            };

            while (true)
            {
                SkipSeparators();
                if (cur >= end)
                {
                    break;
                }

                if (((*cur >= 'A') && (*cur <= 'Z')) || ((*cur >= 'a') && (*cur <= 'z')))
                {
                    command = *cur++;
                }
                else if (command == 0)
                {
                    return false;
                }

                const bool relative = (command >= 'a') && (command <= 'z');
                const char upper = static_cast<char>(command & ~0x20);
                switch (upper)
                {
                    case 'M':
                    {
                        Point2 point{};
                        if (!ReadPoint(relative, point))
                        {
                            return false;
                        }

                        if (hasCurrent && !subpathClosed)
                        {
                            // Leave the previous contour open when a new move starts without a close.
                        }

                        strokeBuilder.AppendMoveTo(ToStrokePoint(point));
                        current = point;
                        subpathStart = point;
                        hasCurrent = true;
                        subpathClosed = false;
                        hasPreviousCubicControl = false;
                        hasPreviousQuadraticControl = false;
                        command = relative ? 'l' : 'L';
                        break;
                    }

                    case 'L':
                    {
                        Point2 point{};
                        if (!ReadPoint(relative, point))
                        {
                            return false;
                        }

                        builder.AddLine(current, point);
                        strokeBuilder.AppendLineTo(ToStrokePoint(point));
                        current = point;
                        hasCurrent = true;
                        hasPreviousCubicControl = false;
                        hasPreviousQuadraticControl = false;
                        break;
                    }

                    case 'H':
                    {
                        float value = 0.0f;
                        if (!ReadFloatValue(value))
                        {
                            return false;
                        }

                        Point2 point = current;
                        point.x = relative ? (current.x + value) : value;
                        builder.AddLine(current, point);
                        strokeBuilder.AppendLineTo(ToStrokePoint(point));
                        current = point;
                        hasCurrent = true;
                        hasPreviousCubicControl = false;
                        hasPreviousQuadraticControl = false;
                        break;
                    }

                    case 'V':
                    {
                        float value = 0.0f;
                        if (!ReadFloatValue(value))
                        {
                            return false;
                        }

                        Point2 point = current;
                        point.y = relative ? (current.y + value) : value;
                        builder.AddLine(current, point);
                        strokeBuilder.AppendLineTo(ToStrokePoint(point));
                        current = point;
                        hasCurrent = true;
                        hasPreviousCubicControl = false;
                        hasPreviousQuadraticControl = false;
                        break;
                    }

                    case 'Q':
                    {
                        Point2 control{};
                        Point2 point{};
                        if (!ReadPoint(relative, control) || !ReadPoint(relative, point))
                        {
                            return false;
                        }

                        builder.AddQuadratic(current, control, point);
                        strokeBuilder.AppendQuadraticTo(ToStrokePoint(control), ToStrokePoint(point));
                        current = point;
                        hasCurrent = true;
                        previousQuadraticControl = control;
                        hasPreviousQuadraticControl = true;
                        hasPreviousCubicControl = false;
                        break;
                    }

                    case 'T':
                    {
                        Point2 point{};
                        if (!ReadPoint(relative, point))
                        {
                            return false;
                        }

                        const Point2 control = hasPreviousQuadraticControl
                            ? ReflectPoint(current, previousQuadraticControl)
                            : current;
                        builder.AddQuadratic(current, control, point);
                        strokeBuilder.AppendQuadraticTo(ToStrokePoint(control), ToStrokePoint(point));
                        current = point;
                        hasCurrent = true;
                        previousQuadraticControl = control;
                        hasPreviousQuadraticControl = true;
                        hasPreviousCubicControl = false;
                        break;
                    }

                    case 'C':
                    {
                        Point2 c1{};
                        Point2 c2{};
                        Point2 point{};
                        if (!ReadPoint(relative, c1)
                            || !ReadPoint(relative, c2)
                            || !ReadPoint(relative, point))
                        {
                            return false;
                        }

                        builder.AddCubic(current, c1, c2, point);
                        strokeBuilder.AppendCubicTo(ToStrokePoint(c1), ToStrokePoint(c2), ToStrokePoint(point));
                        current = point;
                        hasCurrent = true;
                        previousCubicControl = c2;
                        hasPreviousCubicControl = true;
                        hasPreviousQuadraticControl = false;
                        break;
                    }

                    case 'S':
                    {
                        Point2 c2{};
                        Point2 point{};
                        if (!ReadPoint(relative, c2)
                            || !ReadPoint(relative, point))
                        {
                            return false;
                        }

                        const Point2 c1 = hasPreviousCubicControl
                            ? ReflectPoint(current, previousCubicControl)
                            : current;
                        builder.AddCubic(current, c1, c2, point);
                        strokeBuilder.AppendCubicTo(ToStrokePoint(c1), ToStrokePoint(c2), ToStrokePoint(point));
                        current = point;
                        hasCurrent = true;
                        previousCubicControl = c2;
                        hasPreviousCubicControl = true;
                        hasPreviousQuadraticControl = false;
                        break;
                    }

                    case 'Z':
                    {
                        if (hasCurrent)
                        {
                            builder.AddLine(current, subpathStart);
                            strokeBuilder.AppendClose();
                            current = subpathStart;
                            subpathClosed = true;
                            hasPreviousCubicControl = false;
                            hasPreviousQuadraticControl = false;
                        }
                        break;
                    }

                    default:
                        return false;
                }
            }

            outPoints = Move(strokeBuilder.Points());
            outCommands = Move(strokeBuilder.Commands());
            return true;
        }

        bool SvgParser::SkipToClosingTag(StringView tagName)
        {
            while (m_cur < m_end)
            {
                if (((m_end - m_cur) >= 2) && (m_cur[0] == '<') && (m_cur[1] == '/'))
                {
                    const char* save = m_cur + 2;
                    const char* cur = save;
                    while ((cur < m_end) && IsNameChar(*cur))
                    {
                        ++cur;
                    }

                    const StringView name{ save, static_cast<uint32_t>(cur - save) };
                    if (!name.EqualToI(tagName))
                    {
                        return false;
                    }

                    m_cur = cur;
                    SkipWhitespace();
                    return Consume('>');
                }

                ++m_cur;
            }

            return false;
        }

        bool SvgParser::SkipElement(StringView tagName)
        {
            uint32_t depth = 1;
            while (m_cur < m_end)
            {
                if (*m_cur != '<')
                {
                    ++m_cur;
                    continue;
                }

                ++m_cur;
                if ((m_cur < m_end) && ((*m_cur == '!') || (*m_cur == '?')))
                {
                    if (!SkipMarkup())
                    {
                        return false;
                    }

                    continue;
                }

                const bool isClosing = Consume('/');
                const StringView nestedTag = ParseName();
                if (nestedTag.IsEmpty())
                {
                    return false;
                }

                Vector<Attribute> attrs{};
                bool selfClosing = false;
                if (!ParseAttributes(attrs, selfClosing))
                {
                    return false;
                }

                if (isClosing)
                {
                    if (nestedTag.EqualToI(tagName))
                    {
                        --depth;
                        if (depth == 0)
                        {
                            return true;
                        }
                    }
                }
                else if (!selfClosing && nestedTag.EqualToI(tagName))
                {
                    ++depth;
                }
            }

            return false;
        }

        const ParsedShape* SvgParser::FindDefinition(StringView id) const
        {
            for (const ParsedDefinition& definition : m_definitions)
            {
                const StringView definitionId(definition.id.Data(), definition.id.Size());
                if (definitionId.EqualToI(id))
                {
                    return &definition.shape;
                }
            }

            return nullptr;
        }

        const ParsedClipPath* SvgParser::FindClipPath(StringView id) const
        {
            for (const ParsedClipPath& clipPath : m_clipPaths)
            {
                const StringView clipPathId(clipPath.id.Data(), clipPath.id.Size());
                if (clipPathId.EqualToI(id))
                {
                    return &clipPath;
                }
            }

            return nullptr;
        }

        bool BuildCompiledShape(
            CompiledVectorShapeRenderEntry& outShape,
            Vector<PackedCurveTexel>& outCurveTexels,
            Vector<PackedBandTexel>& outBandTexels,
            Vector<CompiledStrokePoint>& outStrokePoints,
            Vector<CompiledStrokeCommand>& outStrokeCommands,
            PackedBandStats& outBandStats,
            const ParsedShape& shape,
            float epsilon)
        {
            outShape = {};
            outBandStats = {};
            if (shape.curves.IsEmpty() && shape.strokeCommands.IsEmpty())
            {
                return false;
            }

            const Point2 localOrigin{ shape.minX, shape.minY };
            outShape.originX = localOrigin.x;
            outShape.originY = localOrigin.y;
            outShape.boundsMinX = 0.0f;
            outShape.boundsMinY = 0.0f;
            outShape.boundsMaxX = Max(shape.maxX - localOrigin.x, 0.0f);
            outShape.boundsMaxY = Max(shape.maxY - localOrigin.y, 0.0f);
            outShape.fillRule = shape.fillRule;
            outShape.firstStrokeCommand = outStrokeCommands.Size();
            outShape.strokeCommandCount = shape.strokeCommands.Size();

            Vector<StrokeSourcePoint> localStrokePoints = shape.strokePoints;
            for (StrokeSourcePoint& point : localStrokePoints)
            {
                point.x -= localOrigin.x;
                point.y -= localOrigin.y;
            }
            AppendCompiledStrokeData(
                outStrokePoints,
                outStrokeCommands,
                Span<const StrokeSourcePoint>(localStrokePoints.Data(), localStrokePoints.Size()),
                Span<const StrokeSourceCommand>(shape.strokeCommands.Data(), shape.strokeCommands.Size()));

            if (!shape.curves.IsEmpty())
            {
                Vector<CurveData> curves = shape.curves;
                for (uint32_t curveIndex = 0; curveIndex < curves.Size(); ++curveIndex)
                {
                    CurveData& curve = curves[curveIndex];
                    curve.p1.x -= localOrigin.x;
                    curve.p1.y -= localOrigin.y;
                    curve.p2.x -= localOrigin.x;
                    curve.p2.y -= localOrigin.y;
                    curve.p3.x -= localOrigin.x;
                    curve.p3.y -= localOrigin.y;
                    curve.minX -= localOrigin.x;
                    curve.minY -= localOrigin.y;
                    curve.maxX -= localOrigin.x;
                    curve.maxY -= localOrigin.y;
                    if (curve.isLineLike)
                    {
                        LineCurvePoint localControl{};
                        if (TryComputeStableLineQuadraticControlPoint(
                                localControl,
                                { curve.p1.x, curve.p1.y },
                                { curve.p3.x, curve.p3.y },
                                DegenerateLineLengthSq))
                        {
                            curve.p2 = { localControl.x, localControl.y };
                        }
                    }
                    curve.curveTexelIndex = outCurveTexels.Size();
                    SvgAppendCurveTexels(outCurveTexels, curve);
                }

                const uint32_t bandCountX = SvgChooseBandCount(curves, false, outShape.boundsMinX, outShape.boundsMaxX, epsilon);
                const uint32_t bandCountY = SvgChooseBandCount(curves, true, outShape.boundsMinY, outShape.boundsMaxY, epsilon);

                Vector<Vector<CurveRef>> xBands{};
                Vector<Vector<CurveRef>> yBands{};
                SvgBuildBandRefs(xBands, curves, false, outShape.boundsMinX, outShape.boundsMaxX, bandCountX, epsilon);
                SvgBuildBandRefs(yBands, curves, true, outShape.boundsMinY, outShape.boundsMaxY, bandCountY, epsilon);

                outShape.bandScaleX = ComputeBandScale(outShape.boundsMinX, outShape.boundsMaxX, bandCountX);
                outShape.bandScaleY = ComputeBandScale(outShape.boundsMinY, outShape.boundsMaxY, bandCountY);
                outShape.bandOffsetX = -outShape.boundsMinX * outShape.bandScaleX;
                outShape.bandOffsetY = -outShape.boundsMinY * outShape.bandScaleY;
                outShape.glyphBandLocX = outBandTexels.Size() % ScribeBandTextureWidth;
                outShape.glyphBandLocY = outBandTexels.Size() / ScribeBandTextureWidth;
                outShape.bandMaxX = bandCountX > 0 ? bandCountX - 1 : 0;
                outShape.bandMaxY = bandCountY > 0 ? bandCountY - 1 : 0;

                const uint32_t glyphBandStart = outBandTexels.Size();
                const PackedBandStats bandStats = AppendPackedBands(
                    outBandTexels,
                    glyphBandStart,
                    yBands,
                    xBands);
                outBandStats = bandStats;
            }

            return true;
        }
    }

    bool BuildCompiledVectorImageData(
        CompiledVectorImageData& out,
        Span<const uint8_t> sourceBytes,
        float flatteningTolerance)
    {
        out = {};
        if (sourceBytes.IsEmpty())
        {
            return false;
        }

        const StringView sourceText(reinterpret_cast<const char*>(sourceBytes.Data()), sourceBytes.Size());
        ParsedImage parsed{};
        SvgParser parser{};
        if (!parser.Parse(parsed, sourceText, flatteningTolerance))
        {
            return false;
        }

        out.bandOverlapEpsilon = DefaultBandOverlapEpsilon;
        out.curveTextureWidth = CurveTextureWidth;
        out.bandTextureWidth = ScribeBandTextureWidth;
        out.viewBoxMinX = parsed.viewBoxMinX;
        out.viewBoxMinY = parsed.viewBoxMinY;
        out.viewBoxWidth = parsed.viewBoxWidth;
        out.viewBoxHeight = parsed.viewBoxHeight;
        out.boundsMinX = parsed.boundsMinX;
        out.boundsMinY = parsed.boundsMinY;
        out.boundsMaxX = parsed.boundsMaxX;
        out.boundsMaxY = parsed.boundsMaxY;
        out.fontFaces = Move(parsed.fontFaces);
        out.textRuns = Move(parsed.textRuns);

        uint32_t totalStrokePointCount = 0;
        uint32_t totalStrokeCommandCount = 0;
        for (const ParsedShape& shape : parsed.shapes)
        {
            totalStrokePointCount += shape.strokePoints.Size();
            totalStrokeCommandCount += shape.strokeCommands.Size();
        }

        out.shapes.Reserve(parsed.shapes.Size());
        out.strokePoints.Reserve(totalStrokePointCount);
        out.strokeCommands.Reserve(totalStrokeCommandCount);
        out.layers = parsed.layers;
        for (uint32_t shapeIndex = 0; shapeIndex < parsed.shapes.Size(); ++shapeIndex)
        {
            const ParsedShape& shape = parsed.shapes[shapeIndex];
            CompiledVectorShapeRenderEntry& compiledShape = out.shapes.EmplaceBack();
            PackedBandStats bandStats{};
            if (!BuildCompiledShape(
                compiledShape,
                out.curveTexels,
                out.bandTexels,
                out.strokePoints,
                out.strokeCommands,
                bandStats,
                shape,
                out.bandOverlapEpsilon))
            {
                return false;
            }
            out.bandHeaderCount += bandStats.headerCount;
            out.emittedBandPayloadTexelCount += bandStats.emittedPayloadTexelCount;
            out.reusedBandCount += bandStats.reusedBandCount;
            out.reusedBandPayloadTexelCount += bandStats.reusedPayloadTexelCount;
        }

        PadCurveTexture(out.curveTexels, out.curveTextureWidth, out.curveTextureHeight);
        PadBandTexture(out.bandTexels, out.bandTextureWidth, out.bandTextureHeight);
        return true;
    }
}
