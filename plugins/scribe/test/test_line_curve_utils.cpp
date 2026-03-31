// Copyright Chad Engler

#include "he/core/test.h"
#include "he/scribe/packed_data.h"

#include "../editor/src/line_curve_utils.h"

using namespace he;

HE_TEST(scribe, line_curve_utils, offsets_axis_aligned_and_diagonal_lines_off_the_midpoint)
{
    using he::scribe::editor::LineCurvePoint;
    using he::scribe::editor::ComputeLineCurveBounds;
    using he::scribe::editor::TryComputeStableLineQuadraticControlPoint;

    constexpr float kDegenerateLineLengthSq = 1.0e-6f;

    auto checkLine = [](const LineCurvePoint& from, const LineCurvePoint& to)
    {
        LineCurvePoint control{};
        HE_EXPECT(TryComputeStableLineQuadraticControlPoint(control, from, to, kDegenerateLineLengthSq));

        const float midX = (from.x + to.x) * 0.5f;
        const float midY = (from.y + to.y) * 0.5f;
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        ComputeLineCurveBounds(minX, minY, maxX, maxY, from, to);

        HE_EXPECT((Abs(control.x - midX) > 1.0e-4f) || (Abs(control.y - midY) > 1.0e-4f));
        HE_EXPECT(minX == Min(from.x, to.x));
        HE_EXPECT(minY == Min(from.y, to.y));
        HE_EXPECT(maxX == Max(from.x, to.x));
        HE_EXPECT(maxY == Max(from.y, to.y));

    };

    checkLine({ 0.0f, 0.0f }, { 0.0f, 2000.0f });
    checkLine({ 0.0f, 0.0f }, { 2000.0f, 0.0f });
    checkLine({ 0.0f, 0.0f }, { 2000.0f, 2000.0f });
    checkLine({ 0.0f, 0.0f }, { 2000.0f, 3000.0f });
}

HE_TEST(scribe, line_curve_utils, keeps_vertical_line_controls_near_the_stem_axis)
{
    using he::scribe::editor::LineCurvePoint;
    using he::scribe::editor::ComputeMinimalHalfFloatOffset;
    using he::scribe::editor::TryComputeStableLineQuadraticControlPoint;

    constexpr float kDegenerateLineLengthSq = 1.0e-6f;
    LineCurvePoint control{};

    HE_EXPECT(TryComputeStableLineQuadraticControlPoint(control, { 0.0f, 0.0f }, { 0.0f, 2000.0f }, kDegenerateLineLengthSq));

    HE_EXPECT_EQ(control.x, ComputeMinimalHalfFloatOffset(0.0f));
    HE_EXPECT(Abs(control.y - 1000.5f) <= 1.0e-4f);
}

HE_TEST(scribe, line_curve_utils, keeps_horizontal_line_controls_near_the_stem_axis)
{
    using he::scribe::editor::LineCurvePoint;
    using he::scribe::editor::ComputeMinimalHalfFloatOffset;
    using he::scribe::editor::TryComputeStableLineQuadraticControlPoint;

    constexpr float kDegenerateLineLengthSq = 1.0e-6f;
    LineCurvePoint control{};

    HE_EXPECT(TryComputeStableLineQuadraticControlPoint(control, { 0.0f, 0.0f }, { 2000.0f, 0.0f }, kDegenerateLineLengthSq));

    HE_EXPECT(Abs(control.x - 1000.5f) <= 1.0e-4f);
    HE_EXPECT_EQ(control.y, -ComputeMinimalHalfFloatOffset(0.0f));
}

HE_TEST(scribe, line_curve_utils, flips_axis_aligned_offsets_with_segment_direction)
{
    using he::scribe::editor::LineCurvePoint;
    using he::scribe::editor::ComputeMinimalHalfFloatOffset;
    using he::scribe::editor::TryComputeStableLineQuadraticControlPoint;

    constexpr float kDegenerateLineLengthSq = 1.0e-6f;

    LineCurvePoint control{};
    HE_EXPECT(TryComputeStableLineQuadraticControlPoint(control, { 0.0f, 2000.0f }, { 0.0f, 0.0f }, kDegenerateLineLengthSq));
    HE_EXPECT_EQ(control.x, -ComputeMinimalHalfFloatOffset(0.0f));

    HE_EXPECT(TryComputeStableLineQuadraticControlPoint(control, { 2000.0f, 0.0f }, { 0.0f, 0.0f }, kDegenerateLineLengthSq));
    HE_EXPECT_EQ(control.y, ComputeMinimalHalfFloatOffset(0.0f));
}
