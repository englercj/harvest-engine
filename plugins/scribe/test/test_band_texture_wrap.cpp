// Copyright Chad Engler

#include "he/core/test.h"
#include "he/scribe/packed_data.h"

#include "he/math/types.h"
#include "he/core/vector.h"

using namespace he;

namespace
{
    uint32_t GetBandTexelIndex(uint32_t x, uint32_t y)
    {
        return (y * scribe::ScribeBandTextureWidth) + x;
    }

    Vec2u CalcBandLocCpu(Vec2u glyphLoc, uint32_t offset)
    {
        Vec2u bandLoc = { glyphLoc.x + offset, glyphLoc.y };
        bandLoc.y += bandLoc.x >> 12u;
        bandLoc.x &= (1u << 12u) - 1u;
        return bandLoc;
    }
}

HE_TEST(scribe, band_texture_wrap, wraps_curve_location_lists_across_texture_rows)
{
    Vector<scribe::PackedBandTexel> bandTexels{};
    bandTexels.Resize(scribe::ScribeBandTextureWidth * 2u, DefaultInit);

    const Vec2u glyphLoc = { scribe::ScribeBandTextureWidth - 2u, 0u };
    bandTexels[GetBandTexelIndex(glyphLoc.x, glyphLoc.y)] = { 3u, 1u };
    bandTexels[GetBandTexelIndex(scribe::ScribeBandTextureWidth - 1u, 0u)] = { 10u, 0u };
    bandTexels[GetBandTexelIndex(0u, 1u)] = { 20u, 0u };
    bandTexels[GetBandTexelIndex(1u, 1u)] = { 30u, 0u };

    const scribe::PackedBandTexel header = bandTexels[GetBandTexelIndex(glyphLoc.x, glyphLoc.y)];
    HE_EXPECT_EQ(header.x, 3u);
    HE_EXPECT_EQ(header.y, 1u);

    const Vec2u listStart = CalcBandLocCpu(glyphLoc, header.y);
    HE_EXPECT_EQ(listStart.x, scribe::ScribeBandTextureWidth - 1u);
    HE_EXPECT_EQ(listStart.y, 0u);

    const Vec2u curveLoc0 = CalcBandLocCpu(listStart, 0u);
    const Vec2u curveLoc1 = CalcBandLocCpu(listStart, 1u);
    const Vec2u curveLoc2 = CalcBandLocCpu(listStart, 2u);

    HE_EXPECT_EQ(curveLoc0.x, scribe::ScribeBandTextureWidth - 1u);
    HE_EXPECT_EQ(curveLoc0.y, 0u);
    HE_EXPECT_EQ(curveLoc1.x, 0u);
    HE_EXPECT_EQ(curveLoc1.y, 1u);
    HE_EXPECT_EQ(curveLoc2.x, 1u);
    HE_EXPECT_EQ(curveLoc2.y, 1u);

    HE_EXPECT_EQ(bandTexels[GetBandTexelIndex(curveLoc0.x, curveLoc0.y)].x, 10u);
    HE_EXPECT_EQ(bandTexels[GetBandTexelIndex(curveLoc1.x, curveLoc1.y)].x, 20u);
    HE_EXPECT_EQ(bandTexels[GetBandTexelIndex(curveLoc2.x, curveLoc2.y)].x, 30u);
}
