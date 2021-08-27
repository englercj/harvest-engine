// Copyright Chad Engler

#include "ulp_diff.h"

#include "he/math/vec3.h"

#include "he/core/test.h"
#include "he/math/constants.h"
#include "he/math/types_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, MakeVec3)
{
    HE_EXPECT_EQ(MakeVec3<float>(Vec2f{ 1, 2 }, 3), (Vec3f{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<float>(Vec4f{ 1, 2, 3, 4 }), (Vec3f{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<float>(Vec2i{ 1, 2 }, 3), (Vec3f{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<float>(Vec3i{ 1, 2, 3 }), (Vec3f{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<float>(Vec4i{ 1, 2, 3, 4 }), (Vec3f{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<float>(Vec2u{ 1, 2 }, 3), (Vec3f{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<float>(Vec3u{ 1, 2, 3 }), (Vec3f{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<float>(Vec4u{ 1, 2, 3, 4 }), (Vec3f{ 1, 2, 3 }));

    HE_EXPECT_EQ(MakeVec3<int32_t>(Vec2f{ 1, 2 }, 3.0f), (Vec3i{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<int32_t>(Vec3f{ 1, 2, 3 }), (Vec3i{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<int32_t>(Vec4f{ 1, 2, 3, 4}), (Vec3i{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<int32_t>(Vec2i{ 1, 2 }, 3), (Vec3i{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<int32_t>(Vec4i{ 1, 2, 3, 4 }), (Vec3i{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<int32_t>(Vec2u{ 1, 2 }, 3), (Vec3i{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<int32_t>(Vec4u{ 1, 2, 3, 4 }), (Vec3i{ 1, 2, 3 }));

    HE_EXPECT_EQ(MakeVec3<uint32_t>(Vec2f{ 1, 2 }, 3.0f), (Vec3u{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<uint32_t>(Vec3f{ 1, 2, 3 }), (Vec3u{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<uint32_t>(Vec4f{ 1, 2, 3, 4}), (Vec3u{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<uint32_t>(Vec2i{ 1, 2 }, 3), (Vec3u{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<uint32_t>(Vec4i{ 1, 2, 3, 4 }), (Vec3u{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<uint32_t>(Vec2u{ 1, 2 }, 3), (Vec3u{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<uint32_t>(Vec4u{ 1, 2, 3, 4 }), (Vec3u{ 1, 2, 3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, GetPointer)
{
    Vec3f v{ 1, 2, 3 };
    HE_EXPECT_EQ_PTR(GetPointer(v), &v.x);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, GetComponent)
{
    HE_EXPECT_EQ(GetComponent(Vec3i{ 1, 2, 3 }, 0), 1);
    HE_EXPECT_EQ(GetComponent(Vec3i{ 1, 2, 3 }, 1), 2);
    HE_EXPECT_EQ(GetComponent(Vec3i{ 1, 2, 3 }, 2), 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, SetComponent)
{
    Vec3i v3i{ 1, 2, 3 };
    HE_EXPECT_EQ(SetComponent(v3i, 0, 0), (Vec3i{ 0, 2, 3 }));
    v3i = { 1, 2, 3 };
    HE_EXPECT_EQ(SetComponent(v3i, 1, 0), (Vec3i{ 1, 0, 3 }));
    v3i = { 1, 2, 3 };
    HE_EXPECT_EQ(SetComponent(v3i, 2, 0), (Vec3i{ 1, 2, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, IsNan)
{
    HE_EXPECT(IsNan(Vec3f{ Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Vec3f{ Float_Nan, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Vec3f{ Float_Nan, 1, Float_Nan }));
    HE_EXPECT(IsNan(Vec3f{ 1, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Vec3f{ Float_Nan, 1, 1 }));
    HE_EXPECT(IsNan(Vec3f{ 1, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Vec3f{ 1, 1, Float_Nan }));
    HE_EXPECT(!IsNan(Vec3f{ Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(!IsNan(Vec3f{ 1, 2, 3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, IsInfinite)
{
    HE_EXPECT(IsInfinite(Vec3f{ Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec3f{ Float_Infinity, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Vec3f{ Float_Infinity, 1, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec3f{ 1, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec3f{ Float_Infinity, 1, 1 }));
    HE_EXPECT(IsInfinite(Vec3f{ 1, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Vec3f{ 1, 1, Float_Infinity }));
    HE_EXPECT(!IsInfinite(Vec3f{ Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(!IsInfinite(Vec3f{ 1, 2, 3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, IsFinite)
{
    HE_EXPECT(IsFinite(Vec3f{ 1, 2, 3 }));
    HE_EXPECT(!IsFinite(Vec3f{ Float_Infinity, 2, 3 }));
    HE_EXPECT(!IsFinite(Vec3f{ 1, Float_Infinity, 3 }));
    HE_EXPECT(!IsFinite(Vec3f{ 1, 2, Float_Infinity }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, IsZeroSafe)
{
    HE_EXPECT(IsZeroSafe(Vec3f{ 1, 2, 3 }));
    HE_EXPECT(IsZeroSafe(Vec3f{ 0, 0, 3 }));
    HE_EXPECT(!IsZeroSafe(Vec3f{ 0, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Negate)
{
    HE_EXPECT_EQ(Negate(Vec3f{ 1, 2, 3 }), (Vec3f{ -1, -2, -3 }));
    HE_EXPECT_EQ(Negate(Vec3i{ 1, 2, 3 }), (Vec3i{ -1, -2, -3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Add)
{
    HE_EXPECT_EQ(Add(Vec3f{ 1, 2, 3 }, Vec3f{ 1, 2, 3 }), (Vec3f{ 2, 4, 6 }));
    HE_EXPECT_EQ(Add(Vec3i{ 1, 2, 3 }, Vec3i{ 1, 2, 3 }), (Vec3i{ 2, 4, 6 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Sub)
{
    HE_EXPECT_EQ(Sub(Vec3f{ 1, 2, 3 }, Vec3f{ 1, 2, 3 }), (Vec3f{ 0, 0, 0 }));
    HE_EXPECT_EQ(Sub(Vec3i{ 1, 2, 3 }, Vec3i{ 1, 2, 3 }), (Vec3i{ 0, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Mul)
{
    HE_EXPECT_EQ(Mul(Vec3f{ 1, 2, 3 }, Vec3f{ 1, 2, 3 }), (Vec3f{ 1, 4, 9 }));
    HE_EXPECT_EQ(Mul(Vec3i{ 1, 2, 3 }, Vec3i{ 1, 2, 3 }), (Vec3i{ 1, 4, 9 }));

    HE_EXPECT_EQ(Mul(Vec3f{ 1, 2, 3 }, 2.0f), (Vec3f{ 2, 4, 6 }));
    HE_EXPECT_EQ(Mul(Vec3i{ 1, 2, 3 }, 2), (Vec3i{ 2, 4, 6 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Div)
{
    HE_EXPECT_EQ_ULP(Div(Vec3f{ 1, 2, 3 }, Vec3f{ 1, 2, 3 }), (Vec3f{ 1, 1, 1 }), 1);
    HE_EXPECT_EQ(Div(Vec3i{ 1, 2, 3 }, Vec3i{ 1, 2, 3 }), (Vec3i{ 1, 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, MulAdd)
{
    HE_EXPECT_EQ(MulAdd(Vec3f{ 1, 2, 3 }, Vec3f{ 1, 2, 3 }, Vec3f{ 1, 2, 3 }), (Vec3f{ 2, 6, 12 }));
    HE_EXPECT_EQ(MulAdd(Vec3i{ 1, 2, 3 }, Vec3i{ 1, 2, 3 }, Vec3i{ 1, 2, 3 }), (Vec3i{ 2, 6, 12 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Lerp)
{
    HE_EXPECT_EQ(Lerp(Vec3f{ 1, 1, 1 }, Vec3f{ 2, 3, 5 }, 0.0f) , (Vec3f{ 1.00f, 1.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec3f{ 1, 1, 1 }, Vec3f{ 2, 3, 5 }, 0.25f), (Vec3f{ 1.25f, 1.5f, 2.0f }));
    HE_EXPECT_EQ(Lerp(Vec3f{ 1, 1, 1 }, Vec3f{ 2, 3, 5 }, 0.5f) , (Vec3f{ 1.50f, 2.0f, 3.0f }));
    HE_EXPECT_EQ(Lerp(Vec3f{ 1, 1, 1 }, Vec3f{ 2, 3, 5 }, 0.75f), (Vec3f{ 1.75f, 2.5f, 4.0f }));
    HE_EXPECT_EQ(Lerp(Vec3f{ 1, 1, 1 }, Vec3f{ 2, 3, 5 }, 1.0f) , (Vec3f{ 2.00f, 3.0f, 5.0f }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, SmoothStep)
{
    HE_EXPECT_EQ(SmoothStep(100.0f, 200.0f, Vec3f{ 150.0f, 200.0f, 300.0f }), (Vec3f{ 0.5f, 1.0f, 1.0f }));
    HE_EXPECT_EQ(SmoothStep(10.0f, 20.0f, Vec3f{ 9.0f, 15.0f, 20.0f }), (Vec3f{ 0.0f, 0.5f, 1.0f }));
    HE_EXPECT_EQ(SmoothStep(10.0f, 20.0f, Vec3f{ 30.0f, 50.0f, 100.0f }), (Vec3f{ 1.0f, 1.0f, 1.0f }));
    HE_EXPECT_EQ(SmoothStep(0.0f, 1.0f, Vec3f{ 0.6f, 0.3f, 0.0f }), (Vec3f{ 0.648000002f, 0.216000021f, 0.0f }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Abs)
{
    HE_EXPECT_EQ(Abs(Vec3f{ 1, -1, 1 }), (Vec3f{ 1, 1, 1 }));
    HE_EXPECT_EQ(Abs(Vec3i{ 1, -1, 1 }), (Vec3i{ 1, 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Rcp)
{
    HE_EXPECT_EQ_ULP(Rcp(Vec3f{ 1, 2, 4 }), (Vec3f{ 1.0f, 0.5f, 0.25f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, RcpSafe)
{
    HE_EXPECT_EQ_ULP(RcpSafe(Vec3f{ 1, 2, 4 }), (Vec3f{ 1.0f, 0.5f, 0.25f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec3f{ Float_Min, 2, 4 }), (Vec3f{ 0.0f, 0.5f, 0.25f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec3f{ 1, Float_Min, 4 }), (Vec3f{ 1.0f, 0.0f, 0.25f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec3f{ 1, 2, Float_Min }), (Vec3f{ 1.0f, 0.5f, 0.0f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Sqrt)
{
    HE_EXPECT_EQ(Sqrt(Vec3f{ 1, 4, 16 }), (Vec3f{ 1, 2, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Rsqrt)
{
    HE_EXPECT_EQ_ULP(Rsqrt(Vec3f{ 1, 4, 16 }), (Vec3f{ 1.0f, 0.5f, 0.25f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Min)
{
    HE_EXPECT_EQ(Min(Vec3f{ 1, 2, 3 }, Vec3f{ 0, 2, 4 }), (Vec3f{ 0, 2, 3 }));
    HE_EXPECT_EQ(Min(Vec3i{ 1, 2, 3 }, Vec3i{ 0, 2, 4 }), (Vec3i{ 0, 2, 3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Max)
{
    HE_EXPECT_EQ(Max(Vec3f{ 1, 2, 3 }, Vec3f{ 0, 2, 4 }), (Vec3f{ 1, 2, 4 }));
    HE_EXPECT_EQ(Max(Vec3i{ 1, 2, 3 }, Vec3i{ 0, 2, 4 }), (Vec3i{ 1, 2, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Clamp)
{
    HE_EXPECT_EQ(Clamp(Vec3f{ 2, 2, 2 }, Vec3f{ 1, 3, 0 }, Vec3f{ 2, 4, 1 }), (Vec3f{ 2, 3, 1 }));
    HE_EXPECT_EQ(Clamp(Vec3i{ 2, 2, 2 }, Vec3i{ 1, 3, 0 }, Vec3i{ 2, 4, 1 }), (Vec3i{ 2, 3, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, HMin)
{
    HE_EXPECT_EQ(HMin(Vec3f{ 1, 3, 5 }), 1);
    HE_EXPECT_EQ(HMin(Vec3i{ 1, 3, 5 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, HMax)
{
    HE_EXPECT_EQ(HMax(Vec3f{ 1, 3, 5 }), 5);
    HE_EXPECT_EQ(HMax(Vec3i{ 1, 3, 5 }), 5);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, HAdd)
{
    HE_EXPECT_EQ(HAdd(Vec3f{ 1, 3, 5 }), 9);
    HE_EXPECT_EQ(HAdd(Vec3i{ 1, 3, 5 }), 9);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Dot)
{
    HE_EXPECT_EQ(Dot(Vec3f{ 1, 2, 3 }, Vec3f{ 0, 1, 2 }), 8);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Cross)
{
    HE_EXPECT_EQ(Cross(Vec3f{ 1, 0, 0 }, Vec3f{ 0, 1, 0 }), (Vec3f{ 0, 0, 1 }));
    HE_EXPECT_EQ(Cross(Vec3f{ 0, 1, 0 }, Vec3f{ 0, 0, 1 }), (Vec3f{ 1, 0, 0 }));
    HE_EXPECT_EQ(Cross(Vec3f{ 0, 0, 1 }, Vec3f{ 1, 0, 0 }), (Vec3f{ 0, 1, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, LenSquared)
{
    HE_EXPECT_EQ(LenSquared(Vec3f{ 1, 2, 3 }), 14);
    HE_EXPECT_EQ(LenSquared(Vec3i{ 1, 2, 3 }), 14);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Len)
{
    HE_EXPECT_EQ(Len(Vec3f{ 1, 2, 3 }), Sqrt(14.f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Normalize)
{
    float x = Rcp(Sqrt(3.f));
    HE_EXPECT_EQ_ULP(Normalize(Vec3f{ 1, 1, 1 }), (Vec3f{ x, x, x }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, NormalizeSafe)
{
    float x = Rcp(Sqrt(3.f));
    HE_EXPECT_EQ_ULP(NormalizeSafe(Vec3f{ 1, 1, 1 }), (Vec3f{ x, x, x }), 1);
    HE_EXPECT_EQ_ULP(NormalizeSafe(Vec3f{ 0, 0, 0 }), (Vec3f{ 0, 0, 0 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, IsNormalized)
{
    float x = Rcp(Sqrt(3.f));
    float y = Rcp(Sqrt(2.f));
    HE_EXPECT(IsNormalized(Vec3f{ 1, 0, 0 }));
    HE_EXPECT(IsNormalized(Vec3f{ y, y, 0 }));
    HE_EXPECT(IsNormalized(Vec3f{ x, x, x }));
    HE_EXPECT(!IsNormalized(Vec3f{ 1, 1, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec3, Operators)
{
    {
        Vec3f v;

        HE_EXPECT_EQ((-Vec3f{ 1, 2, 3 }), (Vec3f{ -1, -2, -3 }));

        v = { 1, 2, 3 };
        v += { 1, 2, 3 };
        HE_EXPECT_EQ(v, (Vec3f{ 2, 4, 6 }));
        v += 1.0f;
        HE_EXPECT_EQ(v, (Vec3f{ 3, 5, 7 }));
        HE_EXPECT_EQ((Vec3f{ 1, 2, 3 } + Vec3f{ 1, 2, 3 }), (Vec3f{ 2, 4, 6 }));
        HE_EXPECT_EQ((Vec3f{ 1, 2, 3 } + 2.0f), (Vec3f{ 3, 4, 5 }));

        v = { 1, 2, 3 };
        v -= { 1, 2, 3 };
        HE_EXPECT_EQ(v, (Vec3f{ 0, 0, 0 }));
        v -= 1.0f;
        HE_EXPECT_EQ(v, (Vec3f{ -1, -1, -1 }));
        HE_EXPECT_EQ((Vec3f{ 1, 2, 3 } - Vec3f{ 1, 2, 3 }), (Vec3f{ 0, 0, 0 }));
        HE_EXPECT_EQ((Vec3f{ 1, 2, 3 } - 2.0f), (Vec3f{ -1, 0, 1 }));

        v = { 1, 2, 3 };
        v *= 2.0f;
        HE_EXPECT_EQ(v, (Vec3f{ 2, 4, 6 }));
        HE_EXPECT_EQ((Vec3f{ 1, 2, 3 } * 2.0f), (Vec3f{ 2, 4, 6 }));

        v = { 1, 2, 3 };
        v *= { 1, 2, 3 };
        HE_EXPECT_EQ(v, (Vec3f{ 1, 4, 9 }));
        HE_EXPECT_EQ((Vec3f{ 1, 2, 3 } * Vec3f{ 1, 2, 3 }), (Vec3f{ 1, 4, 9 }));

        v = { 2, 4, 6 };
        v /= 2.0f;
        HE_EXPECT_EQ(v, (Vec3f{ 1, 2, 3 }));
        v = { 2, 12, 12 };
        v /= { 2, 4, 6};
        HE_EXPECT_EQ(v, (Vec3f{ 1, 3, 2 }));
        HE_EXPECT_EQ((Vec3f{ 2, 4, 6 } / 2.0f), (Vec3f{ 1, 2, 3 }));
        HE_EXPECT_EQ((Vec3f{ 2, 6, 24 } / Vec3f{ 2, 3, 4 }), (Vec3f{ 1, 2, 6 }));

        HE_EXPECT_LT((Vec3f{ 1,1,1 }), (Vec3f{ 2,0,0 }));
        HE_EXPECT_LT((Vec3f{ 1,1,1 }), (Vec3f{ 1,2,0 }));
        HE_EXPECT_LT((Vec3f{ 1,1,1 }), (Vec3f{ 1,1,2 }));

        HE_EXPECT(!(Vec3f{ 1,1,1 } < Vec3f{ 0,2,2 }));
        HE_EXPECT(!(Vec3f{ 1,1,1 } < Vec3f{ 1,0,2 }));
        HE_EXPECT(!(Vec3f{ 1,1,1 } < Vec3f{ 1,1,0 }));

        HE_EXPECT((Vec3f{ 1,1,1 } == Vec3f{ 1,1,1 }));
        HE_EXPECT(!(Vec3f{ 1,1,1 } == Vec3f{ 0,1,1 }));
        HE_EXPECT(!(Vec3f{ 1,1,1 } == Vec3f{ 1,0,1 }));
        HE_EXPECT(!(Vec3f{ 1,1,1 } == Vec3f{ 1,1,0 }));

        HE_EXPECT(!(Vec3f{ 1,1,1 } != Vec3f{ 1,1,1 }));
        HE_EXPECT((Vec3f{ 1,1,1 } != Vec3f{ 0,1,1 }));
        HE_EXPECT((Vec3f{ 1,1,1 } != Vec3f{ 1,0,1 }));
        HE_EXPECT((Vec3f{ 1,1,1 } != Vec3f{ 1,1,0 }));
    }

    {
        Vec3i v;

        HE_EXPECT_EQ((-Vec3i{ 1, 2, 3 }), (Vec3i{ -1, -2, -3 }));

        v = { 1, 2, 3 };
        v += { 1, 2, 3 };
        HE_EXPECT_EQ(v, (Vec3i{ 2, 4, 6 }));
        HE_EXPECT_EQ((Vec3i{ 1, 2, 3 } + Vec3i{ 1, 2, 3 }), (Vec3i{ 2, 4, 6 }));

        v = { 1, 2, 3 };
        v -= { 1, 2, 3 };
        HE_EXPECT_EQ(v, (Vec3i{ 0, 0, 0 }));
        HE_EXPECT_EQ((Vec3i{ 1, 2, 3 } - Vec3i{ 1, 2, 3 }), (Vec3i{ 0, 0, 0 }));

        v = { 1, 2, 3 };
        v *= 2;
        HE_EXPECT_EQ(v, (Vec3i{ 2, 4, 6 }));
        HE_EXPECT_EQ((Vec3i{ 1, 2, 3 } * 2), (Vec3i{ 2, 4, 6 }));

        v = { 1, 2, 3 };
        v *= { 1, 2, 3 };
        HE_EXPECT_EQ(v, (Vec3i{ 1, 4, 9 }));
        HE_EXPECT_EQ((Vec3i{ 1, 2, 3 } * Vec3i{ 1, 2, 3 }), (Vec3i{ 1, 4, 9 }));

        v = { 2, 4, 6 };
        v /= 2;
        HE_EXPECT_EQ(v, (Vec3i{ 1, 2, 3 }));
        HE_EXPECT_EQ((Vec3i{ 2, 4, 6 } / 2), (Vec3i{ 1, 2, 3 }));

        v = { 2, 8, 24 };
        v /= { 2, 4, 8 };
        HE_EXPECT_EQ(v, (Vec3i{ 1, 2, 3}));
        HE_EXPECT_EQ((Vec3i{ 2, 8, 24 } / Vec3i{ 2, 4, 8 }), (Vec3i{ 1, 2, 3 }));


        HE_EXPECT_LT((Vec3i{ 1,1,1 }), (Vec3i{ 2,0,0 }));
        HE_EXPECT_LT((Vec3i{ 1,1,1 }), (Vec3i{ 1,2,0 }));
        HE_EXPECT_LT((Vec3i{ 1,1,1 }), (Vec3i{ 1,1,2 }));

        HE_EXPECT(!(Vec3i{ 1,1,1 } < Vec3i{ 0,2,2 }));
        HE_EXPECT(!(Vec3i{ 1,1,1 } < Vec3i{ 1,0,2 }));
        HE_EXPECT(!(Vec3i{ 1,1,1 } < Vec3i{ 1,1,0 }));

        HE_EXPECT((Vec3i{ 1,1,1 } == Vec3i{ 1,1,1 }));
        HE_EXPECT(!(Vec3i{ 1,1,1 } == Vec3i{ 0,1,1 }));
        HE_EXPECT(!(Vec3i{ 1,1,1 } == Vec3i{ 1,0,1 }));
        HE_EXPECT(!(Vec3i{ 1,1,1 } == Vec3i{ 1,1,0 }));

        HE_EXPECT(!(Vec3i{ 1,1,1 } != Vec3i{ 1,1,1 }));
        HE_EXPECT((Vec3i{ 1,1,1 } != Vec3i{ 0,1,1 }));
        HE_EXPECT((Vec3i{ 1,1,1 } != Vec3i{ 1,0,1 }));
        HE_EXPECT((Vec3i{ 1,1,1 } != Vec3i{ 1,1,0 }));
    }
}
