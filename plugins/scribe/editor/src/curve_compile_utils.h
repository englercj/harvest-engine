// Copyright Chad Engler

#pragma once

#include "band_pack_utils.h"
#include "line_curve_utils.h"

#include "he/scribe/packed_data.h"

#include "he/core/math.h"
#include "he/core/utils.h"

#include <algorithm>

namespace he::scribe::editor::curve_compile
{
    inline constexpr uint32_t CurveTextureWidth = 4096;
    inline constexpr uint32_t MaxBandCount = 8;
    inline constexpr uint32_t MaxCubicSubdivisionDepth = 8;
    inline constexpr float DegenerateLineLengthSq = 1.0e-6f;
    inline constexpr float DegenerateCurveExtent = 1.0e-4f;

    struct Point2
    {
        float x{ 0.0f };
        float y{ 0.0f };
    };

    struct Affine2D
    {
        float m00{ 1.0f };
        float m01{ 0.0f };
        float m10{ 0.0f };
        float m11{ 1.0f };
        float tx{ 0.0f };
        float ty{ 0.0f };
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
        bool isLineLike{ false };
    };

    struct CurveRef
    {
        uint16_t x{ 0 };
        uint16_t y{ 0 };
        float sortKey{ 0.0f };
    };

    inline Point2 MidPoint(const Point2& a, const Point2& b)
    {
        return {
            (a.x + b.x) * 0.5f,
            (a.y + b.y) * 0.5f
        };
    }

    inline Point2 EvaluateQuadratic(const Point2& p0, const Point2& p1, const Point2& p2, float t)
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

    inline Point2 EvaluateCubic(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3, float t)
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

    inline Point2 TransformPoint(const Affine2D& transform, const Point2& point)
    {
        return {
            (transform.m00 * point.x) + (transform.m01 * point.y) + transform.tx,
            (transform.m10 * point.x) + (transform.m11 * point.y) + transform.ty
        };
    }

    inline float DistanceToLineSq(const Point2& point, const Point2& a, const Point2& b)
    {
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        const float lenSq = (dx * dx) + (dy * dy);
        if (lenSq <= 1.0e-8f)
        {
            const float px = point.x - a.x;
            const float py = point.y - a.y;
            return (px * px) + (py * py);
        }

        const float area = ((point.x - a.x) * dy) - ((point.y - a.y) * dx);
        return (area * area) / lenSq;
    }

    inline Point2 ReduceCubicToQuadraticControl(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3)
    {
        const Point2 startControl{
            ((3.0f * p1.x) - p0.x) * 0.5f,
            ((3.0f * p1.y) - p0.y) * 0.5f
        };
        const Point2 endControl{
            ((3.0f * p2.x) - p3.x) * 0.5f,
            ((3.0f * p2.y) - p3.y) * 0.5f
        };
        return MidPoint(startControl, endControl);
    }

    inline float ComputeCubicQuadraticApproximationErrorSq(
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

    inline void AppendCurveTexels(Vector<PackedCurveTexel>& out, const CurveData& curve)
    {
        out.PushBack(PackCurveTexel(curve.p1.x, curve.p1.y, curve.p2.x, curve.p2.y));
        out.PushBack(PackCurveTexel(curve.p3.x, curve.p3.y, 0.0f, 0.0f));
    }

    class CurveBuilder final
    {
    public:
        explicit CurveBuilder(float flatteningTolerance)
            : m_cubicToleranceSq(flatteningTolerance * flatteningTolerance)
        {
        }

        void Clear()
        {
            m_curves.Clear();
            m_transform = {};
        }

        void SetTransform(const Affine2D& transform)
        {
            m_transform = transform;
        }

        void AddLine(const Point2& from, const Point2& to)
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

            AppendCurve(from, { control.x, control.y }, to, true);
        }

        void AddQuadratic(const Point2& p1, const Point2& p2, const Point2& p3)
        {
            AppendCurve(p1, p2, p3, false);
        }

        void AddCubic(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3)
        {
            FlattenCubic(p0, p1, p2, p3, 0);
        }

        Vector<CurveData>& Curves() { return m_curves; }
        const Vector<CurveData>& Curves() const { return m_curves; }

    private:
        void AppendCurve(const Point2& p1, const Point2& p2, const Point2& p3, bool isLineLike)
        {
            const Point2 tp1 = TransformPoint(m_transform, p1);
            const Point2 tp2 = TransformPoint(m_transform, p2);
            const Point2 tp3 = TransformPoint(m_transform, p3);

            float minX = 0.0f;
            float minY = 0.0f;
            float maxX = 0.0f;
            float maxY = 0.0f;
            if (isLineLike)
            {
                minX = Min(tp1.x, tp3.x);
                minY = Min(tp1.y, tp3.y);
                maxX = Max(tp1.x, tp3.x);
                maxY = Max(tp1.y, tp3.y);
            }
            else
            {
                minX = Min(tp1.x, tp2.x, tp3.x);
                minY = Min(tp1.y, tp2.y, tp3.y);
                maxX = Max(tp1.x, tp2.x, tp3.x);
                maxY = Max(tp1.y, tp2.y, tp3.y);
            }

            if (((maxX - minX) <= DegenerateCurveExtent) && ((maxY - minY) <= DegenerateCurveExtent))
            {
                return;
            }

            CurveData& curve = m_curves.EmplaceBack();
            curve.p1 = tp1;
            curve.p2 = tp2;
            curve.p3 = tp3;
            curve.minX = minX;
            curve.minY = minY;
            curve.maxX = maxX;
            curve.maxY = maxY;
            curve.isLineLike = isLineLike;
        }

        void FlattenCubic(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3, uint32_t depth)
        {
            const Point2 quadraticControl = ReduceCubicToQuadraticControl(p0, p1, p2, p3);
            const float errorSq = ComputeCubicQuadraticApproximationErrorSq(p0, p1, p2, p3, quadraticControl);
            if ((depth >= MaxCubicSubdivisionDepth) || (errorSq <= m_cubicToleranceSq))
            {
                AppendCurve(p0, quadraticControl, p3, false);
                return;
            }

            const Point2 p01 = MidPoint(p0, p1);
            const Point2 p12 = MidPoint(p1, p2);
            const Point2 p23 = MidPoint(p2, p3);
            const Point2 p012 = MidPoint(p01, p12);
            const Point2 p123 = MidPoint(p12, p23);
            const Point2 p0123 = MidPoint(p012, p123);

            FlattenCubic(p0, p01, p012, p0123, depth + 1);
            FlattenCubic(p0123, p123, p23, p3, depth + 1);
        }

    private:
        Vector<CurveData> m_curves{};
        Affine2D m_transform{};
        float m_cubicToleranceSq{ 0.0f };
    };

    inline void GetBandRange(
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

    inline uint32_t ChooseBandCount(
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
            for (uint32_t i = 0; i < counts.Size(); ++i) { counts[i] = 0; }

            for (uint32_t curveIndex = 0; curveIndex < curves.Size(); ++curveIndex)
            {
                const CurveData& curve = curves[curveIndex];
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
                    const uint32_t index = static_cast<uint32_t>(bandIndex);
                    ++counts[index];
                }
            }

            uint32_t maxOccupancy = 0;
            for (uint32_t i = 0; i < counts.Size(); ++i)
            {
                maxOccupancy = Max(maxOccupancy, counts[i]);
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

    inline void BuildBandRefs(
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

        for (uint32_t curveIndex = 0; curveIndex < curves.Size(); ++curveIndex)
        {
            const CurveData& curve = curves[curveIndex];
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

        for (uint32_t bandIndex = 0; bandIndex < outBands.Size(); ++bandIndex)
        {
            Vector<CurveRef>& band = outBands[bandIndex];
            if (band.Size() > 1)
            {
                std::sort(band.Data(), band.Data() + band.Size(), [](const CurveRef& a, const CurveRef& b)
                {
                    return a.sortKey > b.sortKey;
                });
            }
        }
    }
}
