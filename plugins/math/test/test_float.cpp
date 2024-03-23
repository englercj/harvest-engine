// Copyright Chad Engler

#include "vec_ulp_diff.h"

#include "he/core/math.h"

#include "he/core/limits.h"
#include "he/core/test.h"

#include <cmath>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, UlpDiff)
{
    HE_EXPECT_EQ_ULP(-0.0f, 0.0f, 0);
    HE_EXPECT_EQ_ULP(0.0f, -0.0f, 0);

    HE_EXPECT_EQ_ULP(0.0f, 1.0f, 0x3f800000);
    HE_EXPECT_EQ_ULP(1.0f, 0.0f, 0x3f800000);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, IsNan)
{
    static_assert(!IsNan(Limits<float>::Min));
    static_assert(!IsNan(Limits<float>::Max));
    static_assert(!IsNan(Limits<float>::Epsilon));
    static_assert(IsNan(Limits<float>::NaN));
    static_assert(IsNan(Limits<float>::SignalingNaN));
    static_assert(IsNan(BitCast<float>(0xffffffff)));

    HE_EXPECT(!IsNan(Limits<float>::Min));
    HE_EXPECT(!IsNan(Limits<float>::Max));
    HE_EXPECT(!IsNan(Limits<float>::Epsilon));
    HE_EXPECT(IsNan(Limits<float>::NaN));
    HE_EXPECT(IsNan(Limits<float>::SignalingNaN));
    HE_EXPECT(IsNan(BitCast<float>(0xffffffff)));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, IsInfinite)
{
    static_assert(!IsInfinite(Limits<float>::Min));
    static_assert(!IsInfinite(Limits<float>::Max));
    static_assert(!IsInfinite(Limits<float>::Epsilon));
    static_assert(IsInfinite(Limits<float>::Infinity));
    static_assert(IsInfinite(Limits<float>::Infinity));

    HE_EXPECT(!IsInfinite(Limits<float>::Min));
    HE_EXPECT(!IsInfinite(Limits<float>::Max));
    HE_EXPECT(!IsInfinite(Limits<float>::Epsilon));
    HE_EXPECT(IsInfinite(Limits<float>::Infinity));
    HE_EXPECT(IsInfinite(Limits<float>::Infinity));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, IsFinite)
{
    static_assert(IsFinite(Limits<float>::Min));
    static_assert(IsFinite(Limits<float>::Max));
    static_assert(IsFinite(Limits<float>::Epsilon));
    static_assert(!IsFinite(Limits<float>::Infinity));
    static_assert(!IsFinite(Limits<float>::Infinity));

    HE_EXPECT(IsFinite(Limits<float>::Min));
    HE_EXPECT(IsFinite(Limits<float>::Max));
    HE_EXPECT(IsFinite(Limits<float>::Epsilon));
    HE_EXPECT(!IsFinite(Limits<float>::Infinity));
    HE_EXPECT(!IsFinite(Limits<float>::Infinity));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, IsZeroSafe)
{
    static_assert(IsZeroSafe(1.0f));
    static_assert(!IsZeroSafe(0.0f));
    static_assert(!IsZeroSafe(-0.0f));
    static_assert(IsZeroSafe(-1.0f));

    HE_EXPECT(IsZeroSafe(1.0f));
    HE_EXPECT(!IsZeroSafe(0.0f));
    HE_EXPECT(!IsZeroSafe(-0.0f));
    HE_EXPECT(IsZeroSafe(-1.0f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Floor)
{
    // https://en.cppreference.com/w/cpp/numeric/math/floor

    // If arg is ±∞, it is returned unmodified
    // We assert on nan & infinity inputs for Ceil
    //HE_EXPECT_EQ(Floor(Limits<float>::Infinity), Limits<float>::Infinity);
    //HE_EXPECT_EQ(Floor(-Limits<float>::Infinity), -Limits<float>::Infinity);

    // If arg is ±0, it is returned, unmodified
    HE_EXPECT_EQ(Floor(0.0f), 0.0f);
    HE_EXPECT_EQ(Floor(-0.0f), -0.0f);

    // If arg is NaN, NaN is returned
    // We assert on nan & infinity inputs for Floor
    //HE_EXPECT_EQ(Floor(Float_Nan), Float_Nan);

    static_assert(Floor(0.999999f) == 0.0f);
    static_assert(Floor(1.0f) == 1.0f);
    static_assert(Floor(1.1f) == 1.0f);
    static_assert(Floor(1.9f) == 1.0f);
    static_assert(Floor(-1.1f) == -2.0f);
    static_assert(Floor(-1.9f) == -2.0f);
    static_assert(Floor(-2.0f) == -2.0f);

    HE_EXPECT_EQ(Floor(1.1f), 1.0f);
    HE_EXPECT_EQ(Floor(1.9f), 1.0f);
    HE_EXPECT_EQ(Floor(-1.1f), -2.0f);
    HE_EXPECT_EQ(Floor(-1.9f), -2.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Ceil)
{
    // https://en.cppreference.com/w/cpp/numeric/math/ceil

    // If arg is ±∞, it is returned unmodified
    // We assert on nan & infinity inputs for Ceil
    //HE_EXPECT_EQ(Ceil(Limits<float>::Infinity), Limits<float>::Infinity);
    //HE_EXPECT_EQ(Ceil(-Limits<float>::Infinity), -Limits<float>::Infinity);

    // If arg is ±0, it is returned, unmodified
    HE_EXPECT_EQ(Ceil(0.0f), 0.0f);
    HE_EXPECT_EQ(Ceil(-0.0f), -0.0f);

    // If arg is NaN, NaN is returned
    // We assert on nan & infinity inputs for Ceil
    //HE_EXPECT_EQ(Ceil(Float_Nan), Float_Nan);

    static_assert(Ceil(0.999999f) == 1.0f);
    static_assert(Ceil(1.0f) == 1.0f);
    static_assert(Ceil(1.1f) == 2.0f);
    static_assert(Ceil(1.9f) == 2.0f);
    static_assert(Ceil(-1.1f) == -1.0f);
    static_assert(Ceil(-1.9f) == -1.0f);
    static_assert(Ceil(-2.0f) == -2.0f);

    HE_EXPECT_EQ(Ceil(1.1f), 2.0f);
    HE_EXPECT_EQ(Ceil(1.9f), 2.0f);
    HE_EXPECT_EQ(Ceil(-1.1f), -1.0f);
    HE_EXPECT_EQ(Ceil(-1.9f), -1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Round)
{
    // https://en.cppreference.com/w/cpp/numeric/math/round

    // If arg is ±∞, it is returned unmodified
    // We assert on nan & infinity inputs for Round
    // HE_EXPECT_EQ(Round(Limits<float>::Infinity), Limits<float>::Infinity);
    // HE_EXPECT_EQ(Round(-Limits<float>::Infinity), -Limits<float>::Infinity);

    // If arg is ±0, it is returned, unmodified
    HE_EXPECT_EQ(Round(0.0f), 0.0f);
    HE_EXPECT_EQ(Round(-0.0f), -0.0f);

    // If arg is NaN, NaN is returned
    // We assert on nan & infinity inputs for Round
    //HE_EXPECT_EQ(Floor(Float_Nan), Float_Nan);

    static_assert(Round(0.999999f) == 1.0f);
    static_assert(Round(1.0f) == 1.0f);
    static_assert(Round(1.1f) == 1.0f);
    static_assert(Round(1.9f) == 2.0f);
    static_assert(Round(-1.1f) == -1.0f);
    static_assert(Round(-1.9f) == -2.0f);
    static_assert(Round(-2.0f) == -2.0f);

    HE_EXPECT_EQ(Round(1.1f), 1.0f);
    HE_EXPECT_EQ(Round(1.9f), 2.0f);
    HE_EXPECT_EQ(Round(-1.1f), -1.0f);
    HE_EXPECT_EQ(Round(-1.9f), -2.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, ToRadians)
{
    static_assert(ToRadians(180.0f) == MathConstants<float>::Pi);

    HE_EXPECT_EQ(ToRadians(180.0f), MathConstants<float>::Pi);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, ToDegrees)
{
    static_assert(ToDegrees(MathConstants<float>::Pi) == 180.0f);

    HE_EXPECT_EQ(ToDegrees(MathConstants<float>::Pi), 180.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Lerp)
{
    static_assert(Lerp(1.0f, 3.0f, 0.0f) == 1.0f);
    static_assert(Lerp(1.0f, 3.0f, 0.25f) == 1.5f);
    static_assert(Lerp(1.0f, 3.0f, 0.5f) == 2.0f);
    static_assert(Lerp(1.0f, 3.0f, 0.75f) == 2.5f);
    static_assert(Lerp(1.0f, 3.0f, 1.0f) == 3.0f);

    HE_EXPECT_EQ(Lerp(1.0f, 3.0f, 0.0f), 1.0f);
    HE_EXPECT_EQ(Lerp(1.0f, 3.0f, 0.25f), 1.5f);
    HE_EXPECT_EQ(Lerp(1.0f, 3.0f, 0.5f), 2.0f);
    HE_EXPECT_EQ(Lerp(1.0f, 3.0f, 0.75f), 2.5f);
    HE_EXPECT_EQ(Lerp(1.0f, 3.0f, 1.0f), 3.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, SmoothStep)
{
    static_assert(SmoothStep(100.0f, 200.0f, 150.0f) == 0.5f);
    static_assert(SmoothStep(10.0f, 20.0f, 9.0f) == 0.0f);
    static_assert(SmoothStep(10.0f, 20.0f, 30.0f) == 1.0f);
    static_assert(IsNearlyEqualULP(SmoothStep(0.0f, 1.0f, 0.6f), 0.648000002f, 1));

    HE_EXPECT_EQ(SmoothStep(100.0f, 200.0f, 150.0f), 0.5f);
    HE_EXPECT_EQ(SmoothStep(10.0f, 20.0f, 9.0f), 0.0f);
    HE_EXPECT_EQ(SmoothStep(10.0f, 20.0f, 30.0f), 1.0f);
    HE_EXPECT_EQ(SmoothStep(0.0f, 1.0f, 0.6f), 0.648000002f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Rcp)
{
    static_assert(IsNearlyEqualULP(Rcp(2.0f), 0.5f, 1));
    static_assert(IsNearlyEqualULP(Rcp(5.0f), 0.2f, 1));

    HE_EXPECT_EQ(Rcp(2.0f), 0.5f);
    HE_EXPECT_EQ(Rcp(5.0f), 0.2f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, RcpSafe)
{
    static_assert(IsNearlyEqualULP(RcpSafe(2.0f), 0.5f, 1));
    static_assert(IsNearlyEqualULP(RcpSafe(5.0f), 0.2f, 1));

    HE_EXPECT_EQ(RcpSafe(2.0f), 0.5f);
    HE_EXPECT_EQ(RcpSafe(5.0f), 0.2f);
    HE_EXPECT_EQ(RcpSafe(0.0f), 0.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Sqrt)
{
    // https://en.cppreference.com/w/cpp/numeric/math/sqrt

    // If the argument is +∞ or ±0, it is returned, unmodified.
    HE_EXPECT_EQ(Sqrt(Limits<float>::Infinity), Limits<float>::Infinity);
    HE_EXPECT_EQ(Sqrt(0.0f), 0.0f);
    HE_EXPECT_EQ(Sqrt(-0.0f), -0.0f);

    // sampling of some known test values
    HE_EXPECT_EQ(Sqrt(4.0f), 2.0f);
    HE_EXPECT_EQ(Sqrt(9.0f), 3.0f);
    HE_EXPECT_EQ(Sqrt(100.0f), 10.0f);
    HE_EXPECT_EQ(Sqrt(459684.0f), 678.0f);
    HE_EXPECT_EQ(Sqrt(12345.0f), 111.1080555135405f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Rsqrt)
{
    HE_EXPECT_EQ(Rsqrt(4.0f), 0.5f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Sin)
{
    // https://en.cppreference.com/w/cpp/numeric/math/sin

    // If the argument is ±0, it is returned unmodified
    HE_EXPECT_EQ(Sin(0.0f), 0.0f);
    HE_EXPECT_EQ(Sin(-0.0f), -0.0f);

    // sampling of some known test values
    HE_EXPECT_EQ(Sin(0.0f), 0.0f);
    HE_EXPECT_EQ(Sin(-0.0f), -0.0f);
    HE_EXPECT_EQ(Sin(MathConstants<float>::PiHalf), 1.0f);

    // sampling of some test values against cmath implementations
    HE_EXPECT_EQ(Sin(0.0f), sinf(0));
    HE_EXPECT_EQ(Sin(MathConstants<float>::PiQuarter), sinf(MathConstants<float>::PiQuarter));
    HE_EXPECT_EQ(Sin(MathConstants<float>::PiHalf), sinf(MathConstants<float>::PiHalf));
    HE_EXPECT_EQ(Sin(MathConstants<float>::Pi), sinf(MathConstants<float>::Pi));
    HE_EXPECT_EQ(Sin(MathConstants<float>::Pi2), sinf(MathConstants<float>::Pi2));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Asin)
{
    // https://en.cppreference.com/w/cpp/numeric/math/asin

    // If the argument is ±0, it is returned unmodified
    HE_EXPECT_EQ(Asin(0.0f), 0.0f);
    HE_EXPECT_EQ(Asin(-0.0f), -0.0f);

    // sampling of some test values against cmath implementations
    HE_EXPECT_EQ(Asin(0.0f), asinf(0.0f));
    HE_EXPECT_EQ(Asin(0.5f), asinf(0.5f));
    HE_EXPECT_EQ(Asin(1.0f), asinf(1.0f));
    HE_EXPECT_EQ(Asin(-1.0f), asinf(-1.0f));
    HE_EXPECT_EQ(Asin(-0.5f), asinf(-0.5f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Cos)
{
    // https://en.cppreference.com/w/cpp/numeric/math/cos

    // If the argument is ±0, the result is 1.0
    HE_EXPECT_EQ(Cos(0.0f), 1.0f);
    HE_EXPECT_EQ(Cos(-0.0f), 1.0f);

    // sampling of some known test values
    HE_EXPECT_EQ(Cos(0.0f), 1.0f);
    HE_EXPECT_EQ(Cos(-0.0f), 1.0f);
    HE_EXPECT_EQ(Cos(MathConstants<float>::Pi), -1.0f);

    // sampling of some test values against cmath implementations
    HE_EXPECT_EQ(Cos(0.0f), cosf(0.0f));
    HE_EXPECT_EQ(Cos(MathConstants<float>::PiQuarter), cosf(MathConstants<float>::PiQuarter));
    HE_EXPECT_EQ(Cos(MathConstants<float>::PiHalf), cosf(MathConstants<float>::PiHalf));
    HE_EXPECT_EQ(Cos(MathConstants<float>::Pi), cosf(MathConstants<float>::Pi));
    HE_EXPECT_EQ(Cos(MathConstants<float>::Pi2), cosf(MathConstants<float>::Pi2));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Acos)
{
    // https://en.cppreference.com/w/cpp/numeric/math/acos

    // If the argument is +1, the value +0 is returned.
    HE_EXPECT_EQ(Acos(1.0f), 0.0f);

    // sampling of some test values against cmath implementations
    HE_EXPECT_EQ(Acos(0.0f), acosf(0.0f));
    HE_EXPECT_EQ(Acos(0.5f), acosf(0.5f));
    HE_EXPECT_EQ(Acos(1.0f), acosf(1.0f));
    HE_EXPECT_EQ(Acos(-1.0f), acosf(-1.0f));
    HE_EXPECT_EQ(Acos(-0.5f), acosf(-0.5f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Tan)
{
    // https://en.cppreference.com/w/cpp/numeric/math/tan

    // If the argument is ±0, it is returned unmodified
    HE_EXPECT_EQ(Tan(0.0f), 0.0f);
    HE_EXPECT_EQ(Tan(-0.0f), -0.0f);

    // sampling of some test values against cmath implementations
    HE_EXPECT_EQ(Tan(0.0f), tanf(0.0f));
    HE_EXPECT_EQ(Tan(MathConstants<float>::PiQuarter), tanf(MathConstants<float>::PiQuarter));
#if !HE_COMPILER_GCC
    // TODO: Somehow this test fails on GCC. Need to look at the generated code to know why...
    HE_EXPECT_EQ(Tan(MathConstants<float>::PiHalf), tanf(MathConstants<float>::PiHalf));
#endif
    HE_EXPECT_EQ(Tan(MathConstants<float>::Pi), tanf(MathConstants<float>::Pi));
    HE_EXPECT_EQ(Tan(MathConstants<float>::Pi2), tanf(MathConstants<float>::Pi2));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Atan)
{
    // https://en.cppreference.com/w/cpp/numeric/math/atan

    // If the argument is ±0, it is returned unmodified
    HE_EXPECT_EQ(Atan(0.0f), 0.0f);
    HE_EXPECT_EQ(Atan(-0.0f), -0.0f);

    // If the argument is +∞, +π/2 is returned
    HE_EXPECT_EQ(Atan(Limits<float>::Infinity), MathConstants<float>::PiHalf);
    HE_EXPECT_EQ(Atan(Limits<float>::Infinity), MathConstants<float>::PiHalf);

    // If the argument is -∞, -π/2 is returned
    HE_EXPECT_EQ(Atan(-Limits<float>::Infinity), -MathConstants<float>::PiHalf);
    HE_EXPECT_EQ(Atan(-Limits<float>::Infinity), -MathConstants<float>::PiHalf);

    // sampling of some test values against cmath implementations
    HE_EXPECT_EQ(Atan(0.0f), atanf(0.0f));
    HE_EXPECT_EQ(Atan(MathConstants<float>::PiQuarter), atanf(MathConstants<float>::PiQuarter));
    HE_EXPECT_EQ(Atan(MathConstants<float>::PiHalf), atanf(MathConstants<float>::PiHalf));
    HE_EXPECT_EQ(Atan(MathConstants<float>::Pi), atanf(MathConstants<float>::Pi));
    HE_EXPECT_EQ(Atan(MathConstants<float>::Pi2), atanf(MathConstants<float>::Pi2));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Atan2)
{
    // https://en.cppreference.com/w/cpp/numeric/math/atan2

    // If y is ±0 and x is negative or -0, ±π is returned
    HE_EXPECT_EQ(Atan2(0.0f, -0.0f), MathConstants<float>::Pi);
    HE_EXPECT_EQ(Atan2(-0.0f, -0.0f), -MathConstants<float>::Pi);
    HE_EXPECT_EQ(Atan2(0.0f, -MathConstants<float>::PiHalf), MathConstants<float>::Pi);
    HE_EXPECT_EQ(Atan2(-0.0f, -MathConstants<float>::PiHalf), -MathConstants<float>::Pi);

    // If y is ±0 and x is positive or +0, ±0 is returned
    HE_EXPECT_EQ(Atan2(0.0f, 0.0f), 0.0f);
    HE_EXPECT_EQ(Atan2(-0.0f, 0.0f), -0.0f);
    HE_EXPECT_EQ(Atan2(0.0f, MathConstants<float>::PiHalf), 0.0f);
    HE_EXPECT_EQ(Atan2(-0.0f, MathConstants<float>::PiHalf), -0.0f);

    // If y is ±∞ and x is finite, ±π/2 is returned
    HE_EXPECT_EQ(Atan2(Limits<float>::Infinity, 0), MathConstants<float>::PiHalf);
    HE_EXPECT_EQ(Atan2(-Limits<float>::Infinity, 0), -MathConstants<float>::PiHalf);
    HE_EXPECT_EQ(Atan2(Limits<float>::Infinity, MathConstants<float>::PiHalf), MathConstants<float>::PiHalf);
    HE_EXPECT_EQ(Atan2(-Limits<float>::Infinity, MathConstants<float>::PiHalf), -MathConstants<float>::PiHalf);

    // If y is ±∞ and x is -∞, ±3π/4 is returned
    constexpr float Pi34 = (3.0f * MathConstants<float>::Pi) / 4.0f;
    HE_EXPECT_EQ(Atan2(Limits<float>::Infinity, -Limits<float>::Infinity), Pi34);
    HE_EXPECT_EQ(Atan2(-Limits<float>::Infinity, -Limits<float>::Infinity), -Pi34);
    HE_EXPECT_EQ(Atan2(Limits<float>::Infinity, -Limits<float>::Infinity), Pi34);
    HE_EXPECT_EQ(Atan2(-Limits<float>::Infinity, -Limits<float>::Infinity), -Pi34);

    // If y is ±∞ and x is +∞, ±π/4 is returned
    HE_EXPECT_EQ(Atan2(Limits<float>::Infinity, Limits<float>::Infinity), MathConstants<float>::PiQuarter);
    HE_EXPECT_EQ(Atan2(-Limits<float>::Infinity, Limits<float>::Infinity), -MathConstants<float>::PiQuarter);
    HE_EXPECT_EQ(Atan2(Limits<float>::Infinity, Limits<float>::Infinity), MathConstants<float>::PiQuarter);
    HE_EXPECT_EQ(Atan2(-Limits<float>::Infinity, Limits<float>::Infinity), -MathConstants<float>::PiQuarter);

    // If x is ±0 and y is negative, -π/2 is returned
    HE_EXPECT_EQ(Atan2(-MathConstants<float>::Pi, 0.0f), -MathConstants<float>::PiHalf);
    HE_EXPECT_EQ(Atan2(-MathConstants<float>::Pi, -0.0f), -MathConstants<float>::PiHalf);

    // If x is ±0 and y is positive, +π/2 is returned
    HE_EXPECT_EQ(Atan2(MathConstants<float>::Pi, 0.0f), MathConstants<float>::PiHalf);
    HE_EXPECT_EQ(Atan2(MathConstants<float>::Pi, -0.0f), MathConstants<float>::PiHalf);

    // If x is -∞ and y is finite and positive, +π is returned
    HE_EXPECT_EQ(Atan2(MathConstants<float>::Pi, -Limits<float>::Infinity), MathConstants<float>::Pi);

    // If x is -∞ and y is finite and negative, -π is returned
    HE_EXPECT_EQ(Atan2(-MathConstants<float>::Pi, -Limits<float>::Infinity), -MathConstants<float>::Pi);

    // If x is +∞ and y is finite and positive, +0 is returned
    HE_EXPECT_EQ(Atan2(MathConstants<float>::Pi, Limits<float>::Infinity), 0.0f);

    // If x is +∞ and y is finite and negative, -0 is returned
    HE_EXPECT_EQ(Atan2(-MathConstants<float>::Pi, Limits<float>::Infinity), -0.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Exp)
{
    // https://en.cppreference.com/w/cpp/numeric/math/exp

    // If the argument is ±0, 1 is returned
    HE_EXPECT_EQ(Exp(0.0f), 1.0f);
    HE_EXPECT_EQ(Exp(-0.0f), 1.0f);

    // If the argument is -∞, +0 is returned
    HE_EXPECT_EQ(Exp(-Limits<float>::Infinity), 0.0f);

    // If the argument is +∞, +∞ is returned
    HE_EXPECT_EQ(Exp(Limits<float>::Infinity), Limits<float>::Infinity);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Ln)
{
    // https://en.cppreference.com/w/cpp/numeric/math/log

    // If the argument is 1, +0 is returned
    HE_EXPECT_EQ(Ln(1.0f), 0.0f);

    // If the argument is +∞, +∞ is returned
    HE_EXPECT_EQ(Ln(Limits<float>::Infinity), Limits<float>::Infinity);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Lb)
{
    // https://en.cppreference.com/w/cpp/numeric/math/log2

    // If the argument is 1, +0 is returned
    HE_EXPECT_EQ(Lb(1.0f), 0.0f);

    // If the argument is +∞, +∞ is returned
    HE_EXPECT_EQ(Lb(Limits<float>::Infinity), Limits<float>::Infinity);

    // sampling of some known test values
    HE_EXPECT_EQ(Lb(65536.0f), 16.0f);
    HE_EXPECT_EQ(Lb(0.125f), -3.0f);
    HE_EXPECT_EQ(Lb(BitCast<float>(0x020)), 9.04165936f);
    HE_EXPECT_EQ(Lb(1.0f), 0.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, LogN)
{
    HE_EXPECT_EQ(LogN(125.0f, 5.0f), 3.0f);
    HE_EXPECT_EQ(LogN(65536.0f, 2.0f), 16.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Pow)
{
    // https://en.cppreference.com/w/cpp/numeric/math/pow

    // pow(+0, exp), where exp is a positive odd integer, returns +0
    HE_EXPECT_EQ(Pow(0.0f, 5.0f), 0.0f);
    HE_EXPECT_EQ(Pow(0.0f, 11.0f), 0.0f);
    HE_EXPECT_EQ(Pow(0.0f, 1.0f), 0.0f);

    // pow(-0, exp), where exp is a positive odd integer, returns -0
    HE_EXPECT_EQ(Pow(-0.0f, 5.0f), -0.0f);
    HE_EXPECT_EQ(Pow(-0.0f, 11.0f), -0.0f);
    HE_EXPECT_EQ(Pow(-0.0f, 1.0f), -0.0f);

    // pow(±0, exp), where exp is positive non-integer or a positive even integer, returns +0
    HE_EXPECT_EQ(Pow(0.0f, 5.5f), 0.0f);
    HE_EXPECT_EQ(Pow(0.0f, 11.1f), 0.0f);
    HE_EXPECT_EQ(Pow(0.0f, 1.6f), 0.0f);
    HE_EXPECT_EQ(Pow(-0.0f, 5.2f), 0.0f);
    HE_EXPECT_EQ(Pow(-0.0f, 11.9f), 0.0f);
    HE_EXPECT_EQ(Pow(-0.0f, 1.3f), 0.0f);
    HE_EXPECT_EQ(Pow(0.0f, 2.0f), 0.0f);
    HE_EXPECT_EQ(Pow(0.0f, 2.6f), 0.0f);
    HE_EXPECT_EQ(Pow(-0.0f, 4.0f), 0.0f);
    HE_EXPECT_EQ(Pow(-0.0f, 4.6f), 0.0f);

    // pow(-1, ±∞) returns 1
    HE_EXPECT_EQ(Pow(-1.0f, Limits<float>::Infinity), 1.0f);
    HE_EXPECT_EQ(Pow(-1.0f, -Limits<float>::Infinity), 1.0f);

    // pow(+1, exp) returns 1 for any exp, even when exp is NaN
    HE_EXPECT_EQ(Pow(1.0f, 0.0f), 1.0f);
    HE_EXPECT_EQ(Pow(1.0f, -0.0f), 1.0f);
    HE_EXPECT_EQ(Pow(1.0f, 10.12f), 1.0f);
    HE_EXPECT_EQ(Pow(1.0f, -67.3f), 1.0f);
    HE_EXPECT_EQ(Pow(1.0f, Limits<float>::Infinity), 1.0f);
    HE_EXPECT_EQ(Pow(1.0f, -Limits<float>::Infinity), 1.0f);

    // pow(base, ±0) returns 1 for any base, even when base is NaN
    HE_EXPECT_EQ(Pow(1.0f, 0.0f), 1.0f);
    HE_EXPECT_EQ(Pow(1.0f, -0.0f), 1.0f);
    HE_EXPECT_EQ(Pow(3.3f, 0.0f), 1.0f);
    HE_EXPECT_EQ(Pow(12.3f, -0.0f), 1.0f);

    // pow(base, -∞) returns +∞ for any |base|<1
    HE_EXPECT_EQ(Pow(0.3f, -Limits<float>::Infinity), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-0.3f, -Limits<float>::Infinity), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-0.5f, -Limits<float>::Infinity), Limits<float>::Infinity);

    // pow(base, -∞) returns +0 for any |base|>1
    HE_EXPECT_EQ(Pow(1.3f, -Limits<float>::Infinity), 0.0f);
    HE_EXPECT_EQ(Pow(-1.3f, -Limits<float>::Infinity), 0.0f);
    HE_EXPECT_EQ(Pow(-5.5f, -Limits<float>::Infinity), 0.0f);

    // pow(base, +∞) returns +0 for any |base|<1
    HE_EXPECT_EQ(Pow(0.3f, Limits<float>::Infinity), 0.0f);
    HE_EXPECT_EQ(Pow(-0.3f, Limits<float>::Infinity), 0.0f);
    HE_EXPECT_EQ(Pow(-0.5f, Limits<float>::Infinity), 0.0f);

    // pow(base, +∞) returns +∞ for any |base|>1
    HE_EXPECT_EQ(Pow(1.3f, Limits<float>::Infinity), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-1.3f, Limits<float>::Infinity), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-5.5f, Limits<float>::Infinity), Limits<float>::Infinity);

    // pow(-∞, exp) returns -0 if exp is a negative odd integer
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -3.0f), -0.0f);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -5.0f), -0.0f);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -125.0f), -0.0f);

    // pow(-∞, exp) returns +0 if exp is a negative non-integer or even integer
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -0.1f), 0.0f);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -3.3f), 0.0f);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -5.8f), 0.0f);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -125.5f), 0.0f);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -2.0f), 0.0f);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -8.0f), 0.0f);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, -128.0f), 0.0f);

    // pow(-∞, exp) returns -∞ if exp is a positive odd integer
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 3.0f), -Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 5.0f), -Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 125.0f), -Limits<float>::Infinity);

    // pow(-∞, exp) returns +∞ if exp is a positive non-integer or even integer
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 0.1f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 3.3f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 5.8f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 125.5f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 2.0f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 8.0f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(-Limits<float>::Infinity, 128.0f), Limits<float>::Infinity);

    // pow(+∞, exp) returns +0 for any negative exp
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -0.0f), 1.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -3.0f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -2.0f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -4.0f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -5.0f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -125.0f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -128.0f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -0.1f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -3.2f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -2.3f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -4.4f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -5.5f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -125.6f), 0.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, -128.7f), 0.0f);

    // pow(+∞, exp) returns +∞ for any positive exp
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 0.0f), 1.0f);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 3.0f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 2.0f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 4.0f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 5.0f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 125.0f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 128.0f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 0.1f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 3.2f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 2.3f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 4.4f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 5.5f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 125.6f), Limits<float>::Infinity);
    HE_EXPECT_EQ(Pow(Limits<float>::Infinity, 128.7f), Limits<float>::Infinity);

    // sampling of some known test values
    HE_EXPECT_EQ(Pow(2.0f, 10.0f), 1024.0f);
    HE_EXPECT_EQ(Pow(2.0f, 0.5f), 1.4142135f);
    HE_EXPECT_EQ(Pow(-2.0f, -3.0f), -0.125f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, float, Fmod)
{
    // https://en.cppreference.com/w/cpp/numeric/math/fmod

    // If x is ±0 and y is not zero, ±0 is returned
    HE_EXPECT_EQ(Fmod(0.0f, 3.0f), 0.0f);
    HE_EXPECT_EQ(Fmod(0.0f, 123.4f), 0.0f);
    HE_EXPECT_EQ(Fmod(0.0f, -3.0f), 0.0f);
    HE_EXPECT_EQ(Fmod(0.0f, -123.4f), 0.0f);
    HE_EXPECT_EQ(Fmod(-0.0f, 3.0f), -0.0f);
    HE_EXPECT_EQ(Fmod(-0.0f, 123.4f), -0.0f);
    HE_EXPECT_EQ(Fmod(-0.0f, -3.0f), -0.0f);
    HE_EXPECT_EQ(Fmod(-0.0f, -123.4f), -0.0f);

    // If y is ±∞ and x is finite, x is returned.
    HE_EXPECT_EQ(Fmod(0.0f, Limits<float>::Infinity), 0.0f);
    HE_EXPECT_EQ(Fmod(-0.0f, Limits<float>::Infinity), -0.0f);
    HE_EXPECT_EQ(Fmod(3.0f, Limits<float>::Infinity), 3.0f);
    HE_EXPECT_EQ(Fmod(-3.0f, Limits<float>::Infinity), -3.0f);
    HE_EXPECT_EQ(Fmod(123.4f, Limits<float>::Infinity), 123.4f);
    HE_EXPECT_EQ(Fmod(-123.4f, Limits<float>::Infinity), -123.4f);

    // sampling of some known test values
    HE_EXPECT_EQ(Fmod(5.1f, 3.0f), 2.1f);
    HE_EXPECT_EQ(Fmod(-5.1f, 3.0f), -2.1f);
    HE_EXPECT_EQ(Fmod(5.1f, -3.0f), 2.1f);
    HE_EXPECT_EQ(Fmod(-5.1f, -3.0f), -2.1f);
    HE_EXPECT_EQ(Fmod(0.0f, 1.0f), 0.0f);
    HE_EXPECT_EQ(Fmod(-0.0f, 1.0f), -0.0f);
    HE_EXPECT_EQ(Fmod(5.1f, Limits<float>::Infinity), 5.1f);
}
