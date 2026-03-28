// Copyright Chad Engler

#pragma once

#include "band_pack_utils.h"
#include "curve_compile_utils.h"
#include "line_curve_utils.h"
#include "outline_compile_data.h"
#include "stroke_compile_utils.h"

#include "he/core/math.h"
#include "he/core/span.h"
#include "he/core/vector.h"

namespace he::scribe::editor
{
    struct CompiledShapeBuildOptions
    {
        float bandOverlapEpsilon{ 0.0f };
        bool normalizeOriginToBoundsMin{ false };
        bool useSingleBandForSmallNonZeroShape{ false };
    };

    template <typename TEntry>
    concept ShapeEntryWithOrigin = requires(TEntry& entry)
    {
        entry.originX = 0.0f;
        entry.originY = 0.0f;
    };

    inline float ComputeBandScale(float minBound, float maxBound, uint32_t bandCount)
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

    inline void PadCurveTexture(Vector<PackedCurveTexel>& texels, uint32_t width, uint32_t& outHeight)
    {
        const uint32_t texelCount = Max(texels.Size(), 1u);
        outHeight = (texelCount + (width - 1)) / width;
        texels.Resize(width * outHeight);
    }

    inline void PadBandTexture(Vector<PackedBandTexel>& texels, uint32_t width, uint32_t& outHeight)
    {
        const uint32_t texelCount = Max(texels.Size(), width);
        outHeight = (texelCount + (width - 1)) / width;
        texels.Resize(width * outHeight);
    }

    inline bool TryComputeOutlineBounds(
        float& outMinX,
        float& outMinY,
        float& outMaxX,
        float& outMaxY,
        Span<const curve_compile::CurveData> curves,
        Span<const StrokeSourcePoint> strokePoints)
    {
        if (!curves.IsEmpty())
        {
            outMinX = curves[0].minX;
            outMinY = curves[0].minY;
            outMaxX = curves[0].maxX;
            outMaxY = curves[0].maxY;
            for (uint32_t curveIndex = 1; curveIndex < curves.Size(); ++curveIndex)
            {
                const curve_compile::CurveData& curve = curves[curveIndex];
                outMinX = Min(outMinX, curve.minX);
                outMinY = Min(outMinY, curve.minY);
                outMaxX = Max(outMaxX, curve.maxX);
                outMaxY = Max(outMaxY, curve.maxY);
            }

            return true;
        }

        if (!strokePoints.IsEmpty())
        {
            outMinX = strokePoints[0].x;
            outMinY = strokePoints[0].y;
            outMaxX = strokePoints[0].x;
            outMaxY = strokePoints[0].y;
            for (uint32_t pointIndex = 1; pointIndex < strokePoints.Size(); ++pointIndex)
            {
                const StrokeSourcePoint& point = strokePoints[pointIndex];
                outMinX = Min(outMinX, point.x);
                outMinY = Min(outMinY, point.y);
                outMaxX = Max(outMaxX, point.x);
                outMaxY = Max(outMaxY, point.y);
            }

            return true;
        }

        outMinX = 0.0f;
        outMinY = 0.0f;
        outMaxX = 0.0f;
        outMaxY = 0.0f;
        return false;
    }

    inline bool ShouldUseSingleBandForShape(Span<const curve_compile::CurveData> curves, FillRule fillRule)
    {
        return (fillRule == FillRule::NonZero) && (curves.Size() <= 4u);
    }

    template <typename TEntry>
    bool BuildCompiledShapeGeometry(
        TEntry& outEntry,
        Vector<PackedCurveTexel>& outCurveTexels,
        Vector<PackedBandTexel>& outBandTexels,
        Vector<CompiledStrokePoint>& outStrokePoints,
        Vector<CompiledStrokeCommand>& outStrokeCommands,
        PackedBandStats& outBandStats,
        Span<const curve_compile::CurveData> curves,
        Span<const StrokeSourcePoint> strokePoints,
        Span<const StrokeSourceCommand> strokeCommands,
        FillRule fillRule,
        const CompiledShapeBuildOptions& options)
    {
        outEntry = {};
        outBandStats = {};
        if (curves.IsEmpty() && strokeCommands.IsEmpty())
        {
            return false;
        }

        float sourceMinX = 0.0f;
        float sourceMinY = 0.0f;
        float sourceMaxX = 0.0f;
        float sourceMaxY = 0.0f;
        if (!TryComputeOutlineBounds(sourceMinX, sourceMinY, sourceMaxX, sourceMaxY, curves, strokePoints))
        {
            return false;
        }

        const curve_compile::Point2 localOrigin = options.normalizeOriginToBoundsMin
            ? curve_compile::Point2{ sourceMinX, sourceMinY }
            : curve_compile::Point2{};

        if constexpr (ShapeEntryWithOrigin<TEntry>)
        {
            outEntry.originX = localOrigin.x;
            outEntry.originY = localOrigin.y;
        }

        outEntry.boundsMinX = sourceMinX - localOrigin.x;
        outEntry.boundsMinY = sourceMinY - localOrigin.y;
        outEntry.boundsMaxX = Max(sourceMaxX - localOrigin.x, 0.0f);
        outEntry.boundsMaxY = Max(sourceMaxY - localOrigin.y, 0.0f);
        outEntry.fillRule = fillRule;
        outEntry.firstStrokeCommand = outStrokeCommands.Size();
        outEntry.strokeCommandCount = strokeCommands.Size();

        Vector<StrokeSourcePoint> localStrokePoints{};
        localStrokePoints.Reserve(strokePoints.Size());
        for (const StrokeSourcePoint& sourcePoint : strokePoints)
        {
            StrokeSourcePoint& point = localStrokePoints.EmplaceBack();
            point.x = sourcePoint.x - localOrigin.x;
            point.y = sourcePoint.y - localOrigin.y;
        }

        AppendCompiledStrokeData(
            outStrokePoints,
            outStrokeCommands,
            Span<const StrokeSourcePoint>(localStrokePoints.Data(), localStrokePoints.Size()),
            strokeCommands);

        if (curves.IsEmpty())
        {
            return true;
        }

        Vector<curve_compile::CurveData> localCurves{};
        localCurves.Reserve(curves.Size());
        for (const curve_compile::CurveData& sourceCurve : curves)
        {
            curve_compile::CurveData& curve = localCurves.EmplaceBack();
            curve = sourceCurve;
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
                LineCurvePoint control{};
                if (TryComputeStableLineQuadraticControlPoint(
                        control,
                        { curve.p1.x, curve.p1.y },
                        { curve.p3.x, curve.p3.y },
                        curve_compile::DegenerateLineLengthSq))
                {
                    curve.p2 = { control.x, control.y };
                }
            }
            curve.curveTexelIndex = outCurveTexels.Size();
            curve_compile::AppendCurveTexels(outCurveTexels, curve);
        }

        const bool useSingleBand =
            options.useSingleBandForSmallNonZeroShape
            && ShouldUseSingleBandForShape(
                Span<const curve_compile::CurveData>(localCurves.Data(), localCurves.Size()),
                fillRule);

        const uint32_t bandCountX = useSingleBand
            ? 1u
            : curve_compile::ChooseBandCount(
                localCurves,
                false,
                outEntry.boundsMinX,
                outEntry.boundsMaxX,
                options.bandOverlapEpsilon);
        const uint32_t bandCountY = useSingleBand
            ? 1u
            : curve_compile::ChooseBandCount(
                localCurves,
                true,
                outEntry.boundsMinY,
                outEntry.boundsMaxY,
                options.bandOverlapEpsilon);

        Vector<Vector<curve_compile::CurveRef>> horizontalBands{};
        Vector<Vector<curve_compile::CurveRef>> verticalBands{};
        curve_compile::BuildBandRefs(
            verticalBands,
            localCurves,
            false,
            outEntry.boundsMinX,
            outEntry.boundsMaxX,
            bandCountX,
            options.bandOverlapEpsilon);
        curve_compile::BuildBandRefs(
            horizontalBands,
            localCurves,
            true,
            outEntry.boundsMinY,
            outEntry.boundsMaxY,
            bandCountY,
            options.bandOverlapEpsilon);

        outEntry.bandScaleX = ComputeBandScale(outEntry.boundsMinX, outEntry.boundsMaxX, bandCountX);
        outEntry.bandScaleY = ComputeBandScale(outEntry.boundsMinY, outEntry.boundsMaxY, bandCountY);
        outEntry.bandOffsetX = -outEntry.boundsMinX * outEntry.bandScaleX;
        outEntry.bandOffsetY = -outEntry.boundsMinY * outEntry.bandScaleY;

        const uint32_t bandStart = outBandTexels.Size();
        outEntry.glyphBandLocX = bandStart & (ScribeBandTextureWidth - 1);
        outEntry.glyphBandLocY = bandStart / ScribeBandTextureWidth;
        outEntry.bandMaxX = bandCountX - 1;
        outEntry.bandMaxY = bandCountY - 1;

        outBandStats = AppendPackedBands(
            outBandTexels,
            bandStart,
            horizontalBands,
            verticalBands);
        return true;
    }
}
