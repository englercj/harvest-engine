// Copyright Chad Engler

#include "vec_ulp_diff.h"

#include "he/math/vec2.h"

#include "he/core/limits.h"
#include "he/core/test.h"
#include "he/math/constants.h"
#include "he/math/types_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, MakeVec2)
{
    HE_EXPECT_EQ(MakeVec2<float>(Vec3f{ 1, 2, 3 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<float>(Vec4f{ 1, 2, 3, 4 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<float>(Vec2i{ 1, 2 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<float>(Vec3i{ 1, 2, 3 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<float>(Vec4i{ 1, 2, 3, 4 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<float>(Vec2u{ 1, 2 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<float>(Vec3u{ 1, 2, 3 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<float>(Vec4u{ 1, 2, 3, 4 }), (Vec2f{ 1, 2 }));

    HE_EXPECT_EQ(MakeVec2<int32_t>(Vec3u{ 1, 2, 3 }), (Vec2i{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<int32_t>(Vec4u{ 1, 2, 3, 4}), (Vec2i{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<int32_t>(Vec3i{ 1, 2, 3 }), (Vec2i{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<int32_t>(Vec4i{ 1, 2, 3, 4}), (Vec2i{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<int32_t>(Vec2f{ 1, 2 }), (Vec2i{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<int32_t>(Vec3f{ 1, 2, 3 }), (Vec2i{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<int32_t>(Vec4f{ 1, 2, 3, 4}), (Vec2i{ 1, 2 }));

    HE_EXPECT_EQ(MakeVec2<uint32_t>(Vec3u{ 1, 2, 3 }), (Vec2u{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<uint32_t>(Vec4u{ 1, 2, 3, 4}), (Vec2u{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<uint32_t>(Vec3i{ 1, 2, 3 }), (Vec2u{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<uint32_t>(Vec4i{ 1, 2, 3, 4}), (Vec2u{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<uint32_t>(Vec2f{ 1, 2 }), (Vec2u{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<uint32_t>(Vec3f{ 1, 2, 3 }), (Vec2u{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<uint32_t>(Vec4f{ 1, 2, 3, 4}), (Vec2u{ 1, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, GetPointer)
{
    Vec2f v{ 1, 2 };
    HE_EXPECT_EQ_PTR(GetPointer(v), &v.x);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, GetComponent)
{
    HE_EXPECT_EQ(GetComponent(Vec2i{ 1, 2 }, 0), 1);
    HE_EXPECT_EQ(GetComponent(Vec2i{ 1, 2 }, 1), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, SetComponent)
{
    Vec2i v{ 1, 2 };
    HE_EXPECT_EQ(SetComponent(v, 0, 0), (Vec2i{ 0, 2 }));
    v = { 1, 2 };
    HE_EXPECT_EQ(SetComponent(v, 1, 0), (Vec2i{ 1, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, IsNan)
{
    HE_EXPECT(IsNan(Vec2f{ Limits<float>::NaN, Limits<float>::NaN }));
    HE_EXPECT(IsNan(Vec2f{ Limits<float>::NaN, 1 }));
    HE_EXPECT(IsNan(Vec2f{ 1, Limits<float>::NaN }));
    HE_EXPECT(!IsNan(Vec2f{ Limits<float>::Infinity, Limits<float>::Infinity }));
    HE_EXPECT(!IsNan(Vec2f{ 1, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, IsInfinite)
{
    HE_EXPECT(IsInfinite(Vec2f{ Limits<float>::Infinity, Limits<float>::Infinity }));
    HE_EXPECT(IsInfinite(Vec2f{ Limits<float>::Infinity, 1 }));
    HE_EXPECT(IsInfinite(Vec2f{ 1, Limits<float>::Infinity }));
    HE_EXPECT(!IsInfinite(Vec2f{ Limits<float>::NaN, Limits<float>::NaN }));
    HE_EXPECT(!IsInfinite(Vec2f{ 1, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, IsFinite)
{
    HE_EXPECT(IsFinite(Vec2f{ 1, 2 }));
    HE_EXPECT(!IsFinite(Vec2f{ Limits<float>::Infinity, 2 }));
    HE_EXPECT(!IsFinite(Vec2f{ 1, Limits<float>::Infinity }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, IsZeroSafe)
{
    HE_EXPECT(IsZeroSafe(Vec2f{ 1, 2 }));
    HE_EXPECT(IsZeroSafe(Vec2f{ 0, 2 }));
    HE_EXPECT(!IsZeroSafe(Vec2f{ 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Negate)
{
    HE_EXPECT_EQ(Negate(Vec2f{ 1, 2 }), (Vec2f{ -1, -2 }));
    HE_EXPECT_EQ(Negate(Vec2i{ 1, 2 }), (Vec2i{ -1, -2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Add)
{
    HE_EXPECT_EQ(Add(Vec2f{ 1, 2 }, Vec2f{ 1, 2 }), (Vec2f{ 2, 4 }));
    HE_EXPECT_EQ(Add(Vec2i{ 1, 2 }, Vec2i{ 1, 2 }), (Vec2i{ 2, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Sub)
{
    HE_EXPECT_EQ(Sub(Vec2f{ 1, 2 }, Vec2f{ 1, 2 }), (Vec2f{ 0, 0 }));
    HE_EXPECT_EQ(Sub(Vec2i{ 1, 2 }, Vec2i{ 1, 2 }), (Vec2i{ 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Mul)
{
    HE_EXPECT_EQ(Mul(Vec2f{ 1, 2 }, Vec2f{ 1, 2 }), (Vec2f{ 1, 4 }));
    HE_EXPECT_EQ(Mul(Vec2i{ 1, 2 }, Vec2i{ 1, 2 }), (Vec2i{ 1, 4 }));

    HE_EXPECT_EQ(Mul(Vec2f{ 1, 2 }, 2.0f), (Vec2f{ 2, 4 }));
    HE_EXPECT_EQ(Mul(Vec2i{ 1, 2 }, 2), (Vec2i{ 2, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Div)
{
    HE_EXPECT_EQ_ULP(Div(Vec2f{ 1, 2 }, Vec2f{ 1, 2 }), (Vec2f{ 1, 1 }), 1);
    HE_EXPECT_EQ(Div(Vec2i{ 1, 2 }, Vec2i{ 1, 2 }), (Vec2i{ 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, MulAdd)
{
    HE_EXPECT_EQ(MulAdd(Vec2f{ 1, 2 }, Vec2f{ 1, 2 }, Vec2f{ 1, 2 }), (Vec2f{ 2, 6 }));
    HE_EXPECT_EQ(MulAdd(Vec2i{ 1, 2 }, Vec2i{ 1, 2 }, Vec2i{ 1, 2 }), (Vec2i{ 2, 6 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Lerp)
{
    HE_EXPECT_EQ(Lerp(Vec2f{ 1, 1 }, Vec2f{ 2, 3 }, 0.0f) , (Vec2f{ 1.00f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec2f{ 1, 1 }, Vec2f{ 2, 3 }, 0.25f), (Vec2f{ 1.25f, 1.5f }));
    HE_EXPECT_EQ(Lerp(Vec2f{ 1, 1 }, Vec2f{ 2, 3 }, 0.5f) , (Vec2f{ 1.50f, 2.0f }));
    HE_EXPECT_EQ(Lerp(Vec2f{ 1, 1 }, Vec2f{ 2, 3 }, 0.75f), (Vec2f{ 1.75f, 2.5f }));
    HE_EXPECT_EQ(Lerp(Vec2f{ 1, 1 }, Vec2f{ 2, 3 }, 1.0f) , (Vec2f{ 2.00f, 3.0f }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, SmoothStep)
{
    HE_EXPECT_EQ(SmoothStep(100.0f, 200.0f, Vec2f{ 150.0f, 200.0f }), (Vec2f{ 0.5f, 1.0f }));
    HE_EXPECT_EQ(SmoothStep(10.0f, 20.0f, Vec2f{ 9.0f, 15.0f }), (Vec2f{ 0.0f, 0.5f }));
    HE_EXPECT_EQ(SmoothStep(10.0f, 20.0f, Vec2f{ 30.0f, 50.0f }), (Vec2f{ 1.0f, 1.0f }));
    HE_EXPECT_EQ(SmoothStep(0.0f, 1.0f, Vec2f{ 0.6f, 0.3f }), (Vec2f{ 0.648000002f, 0.216000021f }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Abs)
{
    HE_EXPECT_EQ(Abs(Vec2f{ 1, -1 }), (Vec2f{ 1, 1 }));
    HE_EXPECT_EQ(Abs(Vec2i{ 1, -1 }), (Vec2i{ 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Rcp)
{
    HE_EXPECT_EQ_ULP(Rcp(Vec2f{ 1, 2 }), (Vec2f{ 1.0f, 0.5f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, RcpSafe)
{
    HE_EXPECT_EQ_ULP(RcpSafe(Vec2f{ 1, 2 }), (Vec2f{ 1.0f, 0.5f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec2f{ Limits<float>::MinPos, 2 }), (Vec2f{ 0.0f, 0.5f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec2f{ 1, Limits<float>::MinPos }), (Vec2f{ 1.0f, 0.0f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Sqrt)
{
    HE_EXPECT_EQ(Sqrt(Vec2f{ 1, 4 }), (Vec2f{ 1, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Rsqrt)
{
    HE_EXPECT_EQ_ULP(Rsqrt(Vec2f{ 1, 4 }), (Vec2f{ 1.0f, 0.5f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Min)
{
    HE_EXPECT_EQ(Min(Vec2f{ 1, 2 }, Vec2f{ 0, 2 }), (Vec2f{ 0, 2 }));
    HE_EXPECT_EQ(Min(Vec2i{ 1, 2 }, Vec2i{ 0, 2 }), (Vec2i{ 0, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Max)
{
    HE_EXPECT_EQ(Max(Vec2f{ 1, 2 }, Vec2f{ 0, 2 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(Max(Vec2i{ 1, 2 }, Vec2i{ 0, 2 }), (Vec2i{ 1, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Clamp)
{
    HE_EXPECT_EQ(Clamp(Vec2f{ 2, 2 }, Vec2f{ 1, 3 }, Vec2f{ 2, 4 }), (Vec2f{ 2, 3 }));
    HE_EXPECT_EQ(Clamp(Vec2i{ 2, 2 }, Vec2i{ 1, 3 }, Vec2i{ 2, 4 }), (Vec2i{ 2, 3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, HMin)
{
    HE_EXPECT_EQ(HMin(Vec2f{ 1, 3 }), 1);
    HE_EXPECT_EQ(HMin(Vec2i{ 1, 3 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, HMax)
{
    HE_EXPECT_EQ(HMax(Vec2f{ 1, 3 }), 3);
    HE_EXPECT_EQ(HMax(Vec2i{ 1, 3 }), 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, HAdd)
{
    HE_EXPECT_EQ(HAdd(Vec2f{ 1, 3 }), 4);
    HE_EXPECT_EQ(HAdd(Vec2i{ 1, 3 }), 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Dot)
{
    HE_EXPECT_EQ(Dot(Vec2f{ 1, 2 }, Vec2f{ 0, 1 }), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, LenSquared)
{
    HE_EXPECT_EQ(LenSquared(Vec2f{ 1, 2 }), 5);
    HE_EXPECT_EQ(LenSquared(Vec2i{ 1, 2 }), 5);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Len)
{
    HE_EXPECT_EQ(Len(Vec2f{ 1, 2 }), Sqrt(5.f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Normalize)
{
    float x = Rcp(Sqrt(2.f));
    HE_EXPECT_EQ_ULP(Normalize(Vec2f{ 1, 1 }), (Vec2f{ x, x }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, NormalizeSafe)
{
    float x = Rcp(Sqrt(2.f));
    HE_EXPECT_EQ_ULP(NormalizeSafe(Vec2f{ 1, 1 }), (Vec2f{ x, x }), 1);
    HE_EXPECT_EQ_ULP(NormalizeSafe(Vec2f{ 0, 0 }), (Vec2f{ 0, 0 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, IsNormalized)
{
    float x = Rcp(Sqrt(2.f));
    HE_EXPECT(IsNormalized(Vec2f{ 1, 0 }));
    HE_EXPECT(IsNormalized(Vec2f{ x, x }));
    HE_EXPECT(!IsNormalized(Vec2f{ 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec2, Operators)
{
    {
        Vec2f v;

        HE_EXPECT_EQ((-Vec2f{ 1, 2 }), (Vec2f{ -1, -2 }));

        v = { 1, 2 };
        v += { 1, 2 };
        HE_EXPECT_EQ(v, (Vec2f{ 2, 4 }));
        v += 1.0f;
        HE_EXPECT_EQ(v, (Vec2f{ 3, 5 }));
        HE_EXPECT_EQ((Vec2f{ 1, 2 } + Vec2f{ 1, 2 }), (Vec2f{ 2, 4 }));
        HE_EXPECT_EQ((Vec2f{ 1, 2 } + 1.0f), (Vec2f{ 2, 3 }));

        v = { 1, 2 };
        v -= { 1, 2 };
        HE_EXPECT_EQ(v, (Vec2f{ 0, 0 }));
        HE_EXPECT_EQ((Vec2f{ 1, 2 } - Vec2f{ 1, 2 }), (Vec2f{ 0, 0 }));
        HE_EXPECT_EQ((Vec2f{ 1, 2 } - 1.0f), (Vec2f{ 0, 1 }));

        v = { 1, 2 };
        v *= 2.0f;
        HE_EXPECT_EQ(v, (Vec2f{ 2, 4 }));
        v -= 1.0f;
        HE_EXPECT_EQ(v, (Vec2f{ 1, 3 }));
        HE_EXPECT_EQ((Vec2f{ 1, 2 } * 2.0f), (Vec2f{ 2, 4 }));

        v = { 1, 2 };
        v *= { 1, 2 };
        HE_EXPECT_EQ(v, (Vec2f{ 1, 4 }));
        HE_EXPECT_EQ((Vec2f{ 1, 2 } * Vec2f{ 1, 2 }), (Vec2f{ 1, 4 }));

        v = { 2, 4 };
        v /= 2.0f;
        HE_EXPECT_EQ(v, (Vec2f{ 1, 2 }));
        v = { 2, 4 };
        v /= { 2, 4 };
        HE_EXPECT_EQ(v, (Vec2f{ 1, 1 }));
        HE_EXPECT_EQ((Vec2f{ 2, 4} / 2.0f), (Vec2f{ 1, 2}));
        HE_EXPECT_EQ((Vec2f{ 2, 16 } / Vec2f{ 2, 4 }), (Vec2f{ 1, 4 }));

        HE_EXPECT_LT((Vec2f{ 1,1 }), (Vec2f{ 2,0 }));
        HE_EXPECT_LT((Vec2f{ 1,1 }), (Vec2f{ 1,2 }));

        HE_EXPECT(!(Vec2f{ 1,1 } < Vec2f{ 0,2 }));
        HE_EXPECT(!(Vec2f{ 1,1 } < Vec2f{ 1,0 }));

        HE_EXPECT((Vec2f{ 1,1 } == Vec2f{ 1,1 }));
        HE_EXPECT(!(Vec2f{ 1,1 } == Vec2f{ 0,1 }));
        HE_EXPECT(!(Vec2f{ 1,1 } == Vec2f{ 1,0 }));

        HE_EXPECT(!(Vec2f{ 1,1 } != Vec2f{ 1,1 }));
        HE_EXPECT((Vec2f{ 1,1 } != Vec2f{ 0,1 }));
        HE_EXPECT((Vec2f{ 1,1 } != Vec2f{ 1,0 }));
    }

    {
        Vec2i v;

        HE_EXPECT_EQ((-Vec2i{ 1, 2 }), (Vec2i{ -1, -2 }));

        v = { 1, 2 };
        v += { 1, 2 };
        HE_EXPECT_EQ(v, (Vec2i{ 2, 4 }));
        HE_EXPECT_EQ((Vec2i{ 1, 2 } + Vec2i{ 1, 2 }), (Vec2i{ 2, 4 }));

        v = { 1, 2 };
        v -= { 1, 2 };
        HE_EXPECT_EQ(v, (Vec2i{ 0, 0 }));
        HE_EXPECT_EQ((Vec2i{ 1, 2 } - Vec2i{ 1, 2 }), (Vec2i{ 0, 0 }));

        v = { 1, 2 };
        v *= 2;
        HE_EXPECT_EQ(v, (Vec2i{ 2, 4 }));
        HE_EXPECT_EQ((Vec2i{ 1, 2 } * 2), (Vec2i{ 2, 4 }));

        v = { 1, 2 };
        v *= { 1, 2 };
        HE_EXPECT_EQ(v, (Vec2i{ 1, 4 }));
        HE_EXPECT_EQ((Vec2i{ 1, 2 } * Vec2i{ 1, 2 }), (Vec2i{ 1, 4 }));

        v = { 2, 4 };
        v /= 2;
        HE_EXPECT_EQ(v, (Vec2i{ 1, 2 }));
        HE_EXPECT_EQ((Vec2i{ 2, 4 } / 2), (Vec2i{ 1, 2 }));

        v = { 2, 8 };
        v /= { 2, 4 };
        HE_EXPECT_EQ(v, (Vec2i{ 1, 2 }));
        HE_EXPECT_EQ((Vec2i{ 2, 8 } / Vec2i{ 2, 4 }), (Vec2i{ 1, 2 }));

        HE_EXPECT_LT((Vec2i{ 1,1 }), (Vec2i{ 2,0 }));
        HE_EXPECT_LT((Vec2i{ 1,1 }), (Vec2i{ 1,2 }));

        HE_EXPECT(!(Vec2i{ 1,1 } < Vec2i{ 0,2 }));
        HE_EXPECT(!(Vec2i{ 1,1 } < Vec2i{ 1,0 }));

        HE_EXPECT((Vec2i{ 1,1 } == Vec2i{ 1,1 }));
        HE_EXPECT(!(Vec2i{ 1,1 } == Vec2i{ 0,1 }));
        HE_EXPECT(!(Vec2i{ 1,1 } == Vec2i{ 1,0 }));

        HE_EXPECT(!(Vec2i{ 1,1 } != Vec2i{ 1,1 }));
        HE_EXPECT((Vec2i{ 1,1 } != Vec2i{ 0,1 }));
        HE_EXPECT((Vec2i{ 1,1 } != Vec2i{ 1,0 }));
    }
}
