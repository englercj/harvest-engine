// Copyright Chad Engler

#include "band_pack_utils.h"

#include "he/core/test.h"

using namespace he;
using namespace he::scribe;
using namespace he::scribe::editor;

namespace
{
    struct TestBandRef
    {
        uint16_t x{ 0 };
        uint16_t y{ 0 };
    };
}

HE_TEST(scribe, band_pack_utils, reuses_only_identical_band_payloads)
{
    Vector<Vector<TestBandRef>> horizontalBands{};
    horizontalBands.Resize(3);
    horizontalBands[0].PushBack({ 10, 1 });
    horizontalBands[0].PushBack({ 11, 1 });
    horizontalBands[0].PushBack({ 12, 1 });
    horizontalBands[1].PushBack({ 11, 1 });
    horizontalBands[1].PushBack({ 12, 1 });
    horizontalBands[2].PushBack({ 10, 1 });
    horizontalBands[2].PushBack({ 11, 1 });
    horizontalBands[2].PushBack({ 12, 1 });

    Vector<Vector<TestBandRef>> verticalBands{};
    verticalBands.Resize(1);

    Vector<PackedBandTexel> bandTexels{};
    const PackedBandStats stats = AppendPackedBands(bandTexels, 0, horizontalBands, verticalBands);

    HE_EXPECT_EQ(stats.headerCount, 4u);
    HE_EXPECT_EQ(stats.emittedPayloadTexelCount, 5u);
    HE_EXPECT_EQ(stats.reusedBandCount, 1u);
    HE_EXPECT_EQ(stats.reusedPayloadTexelCount, 3u);
    HE_EXPECT_EQ(bandTexels.Size(), 9u);

    HE_EXPECT_EQ(bandTexels[0].x, 3u);
    HE_EXPECT_EQ(bandTexels[0].y, 4u);
    HE_EXPECT_EQ(bandTexels[1].x, 2u);
    HE_EXPECT_EQ(bandTexels[1].y, 7u);
    HE_EXPECT_EQ(bandTexels[2].x, 3u);
    HE_EXPECT_EQ(bandTexels[2].y, 4u);
    HE_EXPECT_EQ(bandTexels[3].x, 0u);
    HE_EXPECT_EQ(bandTexels[3].y, 9u);

    HE_EXPECT_EQ(bandTexels[4].x, 10u);
    HE_EXPECT_EQ(bandTexels[4].y, 1u);
    HE_EXPECT_EQ(bandTexels[5].x, 11u);
    HE_EXPECT_EQ(bandTexels[5].y, 1u);
    HE_EXPECT_EQ(bandTexels[6].x, 12u);
    HE_EXPECT_EQ(bandTexels[6].y, 1u);
    HE_EXPECT_EQ(bandTexels[7].x, 11u);
    HE_EXPECT_EQ(bandTexels[7].y, 1u);
    HE_EXPECT_EQ(bandTexels[8].x, 12u);
    HE_EXPECT_EQ(bandTexels[8].y, 1u);
}
