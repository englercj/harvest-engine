// Copyright Chad Engler

#include "he/core/test.h"

#include "../editor/src/curve_compile_utils.h"

using namespace he;

HE_TEST(scribe, curve_compile_utils, reduces_cubics_to_quadratic_segments_instead_of_line_segments)
{
    using namespace he::scribe::editor::curve_compile;

    CurveBuilder builder(0.01f);
    builder.AddCubic(
        { 0.0f, 0.0f },
        { 0.0f, 120.0f },
        { 120.0f, 120.0f },
        { 120.0f, 0.0f });

    const Vector<CurveData>& curves = builder.Curves();
    HE_EXPECT_GT(curves.Size(), 0u);

    bool hasQuadraticSegment = false;
    for (const CurveData& curve : curves)
    {
        if (!curve.isLineLike)
        {
            hasQuadraticSegment = true;
            break;
        }
    }

    HE_EXPECT(hasQuadraticSegment);
}
