// Copyright Chad Engler

#include "vec_ulp_diff.h"

#include "he/math/vec4.h"

#include "he/core/test.h"
#include "he/math/constants.h"
#include "he/math/types_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, MakeVec4)
{
    HE_EXPECT_EQ(MakeVec4<float>(Vec2f{ 1, 2 }, 3, 4), (Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<float>(Vec3f{ 1, 2, 3 }, 4), (Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<float>(Vec2i{ 1, 2 }, 3, 4), (Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<float>(Vec3i{ 1, 2, 3 }, 4), (Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<float>(Vec4i{ 1, 2, 3, 4 }), (Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<float>(Vec2u{ 1, 2 }, 3, 4), (Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<float>(Vec3u{ 1, 2, 3 }, 4), (Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<float>(Vec4u{ 1, 2, 3, 4 }), (Vec4f{ 1, 2, 3, 4 }));

    HE_EXPECT_EQ(MakeVec4<int32_t>(Vec2f{ 1, 2 }, 3, 4), (Vec4i{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<int32_t>(Vec3f{ 1, 2, 3 }, 4), (Vec4i{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<int32_t>(Vec4f{ 1, 2, 3, 4 }), (Vec4i{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<int32_t>(Vec2i{ 1, 2 }, 3, 4), (Vec4i{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<int32_t>(Vec3i{ 1, 2, 3 }, 4), (Vec4i{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<int32_t>(Vec2u{ 1, 2 }, 3, 4), (Vec4i{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<int32_t>(Vec3u{ 1, 2, 3 }, 4), (Vec4i{ 1, 2, 3, 4 }));

    HE_EXPECT_EQ(MakeVec4<uint32_t>(Vec2f{ 1, 2 }, 3, 4), (Vec4u{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<uint32_t>(Vec3f{ 1, 2, 3 }, 4), (Vec4u{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<uint32_t>(Vec4f{ 1, 2, 3, 4 }), (Vec4u{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<uint32_t>(Vec2i{ 1, 2 }, 3, 4), (Vec4u{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<uint32_t>(Vec3i{ 1, 2, 3 }, 4), (Vec4u{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<uint32_t>(Vec2u{ 1, 2 }, 3, 4), (Vec4u{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<uint32_t>(Vec3u{ 1, 2, 3 }, 4), (Vec4u{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, GetPointer)
{
    Vec4f v{ 1, 2, 3, 4 };
    HE_EXPECT_EQ_PTR(GetPointer(v), &v.x);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, GetComponent)
{
    HE_EXPECT_EQ(GetComponent(Vec4i{ 1, 2, 3, 4 }, 0), 1);
    HE_EXPECT_EQ(GetComponent(Vec4i{ 1, 2, 3, 4 }, 1), 2);
    HE_EXPECT_EQ(GetComponent(Vec4i{ 1, 2, 3, 4 }, 2), 3);
    HE_EXPECT_EQ(GetComponent(Vec4i{ 1, 2, 3, 4 }, 3), 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, SetComponent)
{
    Vec4i v4i{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(SetComponent(v4i, 0, 0), (Vec4i{ 0, 2, 3, 4 }));
    v4i = { 1, 2, 3, 4 };
    HE_EXPECT_EQ(SetComponent(v4i, 1, 0), (Vec4i{ 1, 0, 3, 4 }));
    v4i = { 1, 2, 3, 4 };
    HE_EXPECT_EQ(SetComponent(v4i, 2, 0), (Vec4i{ 1, 2, 0, 4 }));
    v4i = { 1, 2, 3, 4 };
    HE_EXPECT_EQ(SetComponent(v4i, 3, 0), (Vec4i{ 1, 2, 3, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, IsNan)
{
    HE_EXPECT(IsNan(Vec4f{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Vec4f{ Float_Nan, Float_Nan, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Vec4f{ Float_Nan, Float_Nan, 1, Float_Nan }));
    HE_EXPECT(IsNan(Vec4f{ Float_Nan, 1, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Vec4f{ 1, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Vec4f{ Float_Nan, 1, 1, 1 }));
    HE_EXPECT(IsNan(Vec4f{ 1, Float_Nan, 1, 1 }));
    HE_EXPECT(IsNan(Vec4f{ 1, 1, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Vec4f{ 1, 1, 1, Float_Nan }));
    HE_EXPECT(!IsNan(Vec4f{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(!IsNan(Vec4f{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, IsInfinite)
{
    HE_EXPECT(IsInfinite(Vec4f{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec4f{ Float_Infinity, Float_Infinity, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Vec4f{ Float_Infinity, Float_Infinity, 1, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec4f{ Float_Infinity, 1, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec4f{ 1, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec4f{ Float_Infinity, 1, 1, 1 }));
    HE_EXPECT(IsInfinite(Vec4f{ 1, Float_Infinity, 1, 1 }));
    HE_EXPECT(IsInfinite(Vec4f{ 1, 1, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Vec4f{ 1, 1, 1, Float_Infinity }));
    HE_EXPECT(!IsInfinite(Vec4f{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(!IsInfinite(Vec4f{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, IsFinite)
{
    HE_EXPECT(IsFinite(Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT(!IsFinite(Vec4f{ Float_Infinity, 2, 3, 4 }));
    HE_EXPECT(!IsFinite(Vec4f{ 1, Float_Infinity, 3, 4 }));
    HE_EXPECT(!IsFinite(Vec4f{ 1, 2, Float_Infinity, 4 }));
    HE_EXPECT(!IsFinite(Vec4f{ 1, 2, 3, Float_Infinity }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, IsZeroSafe)
{
    HE_EXPECT(IsZeroSafe(Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT(IsZeroSafe(Vec4f{ 0, 0, 0, 4 }));
    HE_EXPECT(!IsZeroSafe(Vec4f{ 0, 0, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Negate)
{
    HE_EXPECT_EQ(Negate(Vec4f{ 1, 2, 3, 4 }), (Vec4f{ -1, -2, -3, -4 }));
    HE_EXPECT_EQ(Negate(Vec4i{ 1, 2, 3, 4 }), (Vec4i{ -1, -2, -3, -4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Add)
{
    HE_EXPECT_EQ(Add(Vec4i{ 1, 2, 3, 4 }, Vec4i { 1, 2, 3, 4 }), (Vec4i{ 2, 4, 6, 8 }));
    HE_EXPECT_EQ(Add(Vec4f{ 1, 2, 3, 4 }, Vec4f{ 1, 2, 3, 4 }), (Vec4f{ 2, 4, 6, 8 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Sub)
{
    HE_EXPECT_EQ(Sub(Vec4f{ 1, 2, 3, 4 }, Vec4f{ 1, 2, 3, 4 }), (Vec4f{ 0, 0, 0, 0 }));
    HE_EXPECT_EQ(Sub(Vec4i{ 1, 2, 3, 4 }, Vec4i{ 1, 2, 3, 4}), (Vec4i{ 0, 0, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Mul)
{
    HE_EXPECT_EQ(Mul(Vec4f{ 1, 2, 3, 4 }, Vec4f{ 1, 2, 3, 4 }), (Vec4f{ 1, 4, 9, 16 }));
    HE_EXPECT_EQ(Mul(Vec4i{ 1, 2, 3, 4 }, Vec4i{ 1, 2, 3, 4 }), (Vec4i{ 1, 4, 9, 16 }));

    HE_EXPECT_EQ(Mul(Vec4f{ 1, 2, 3, 4 }, 2.0f), (Vec4f{ 2, 4, 6, 8 }));
    HE_EXPECT_EQ(Mul(Vec4i{ 1, 2, 3, 4 }, 2), (Vec4i{ 2, 4, 6, 8 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Div)
{
    HE_EXPECT_EQ_ULP(Div(Vec4f{ 1, 2, 3, 4 }, Vec4f{ 1, 2, 3, 4 }), (Vec4f{ 1, 1, 1, 1 }), 1);
    HE_EXPECT_EQ(Div(Vec4i{ 1, 2, 3, 4 }, Vec4i{ 1, 2, 3, 4 }), (Vec4i{ 1, 1, 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, MulAdd)
{
    HE_EXPECT_EQ(MulAdd(Vec4f{ 1, 2, 3, 4 }, Vec4f{ 1, 2, 3, 4 }, Vec4f{ 1, 2, 3, 4 }), (Vec4f{ 2, 6, 12, 20 }));
    HE_EXPECT_EQ(MulAdd(Vec4i{ 1, 2, 3, 4 }, Vec4i{ 1, 2, 3, 4 }, Vec4i{ 1, 2, 3, 4 }), (Vec4i{ 2, 6, 12, 20 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Lerp)
{
    HE_EXPECT_EQ(Lerp(Vec4f{ 1, 1, 1, 1 }, Vec4f{ 2, 3, 5, 1 }, 0.0f) , (Vec4f{ 1.00f, 1.0f, 1.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec4f{ 1, 1, 1, 1 }, Vec4f{ 2, 3, 5, 1 }, 0.25f), (Vec4f{ 1.25f, 1.5f, 2.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec4f{ 1, 1, 1, 1 }, Vec4f{ 2, 3, 5, 1 }, 0.5f) , (Vec4f{ 1.50f, 2.0f, 3.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec4f{ 1, 1, 1, 1 }, Vec4f{ 2, 3, 5, 1 }, 0.75f), (Vec4f{ 1.75f, 2.5f, 4.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec4f{ 1, 1, 1, 1 }, Vec4f{ 2, 3, 5, 1 }, 1.0f) , (Vec4f{ 2.00f, 3.0f, 5.0f, 1.0f }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, SmoothStep)
{
    HE_EXPECT_EQ(SmoothStep(100.0f, 200.0f, Vec4f{ 150.0f, 200.0f, 300.0f, 100.0f }), (Vec4f{ 0.5f, 1.0f, 1.0f, 0.0f }));
    HE_EXPECT_EQ(SmoothStep(10.0f, 20.0f, Vec4f{ 9.0f, 15.0f, 20.0f, 10.0f }), (Vec4f{ 0.0f, 0.5f, 1.0f, 0.0f }));
    HE_EXPECT_EQ(SmoothStep(10.0f, 20.0f, Vec4f{ 30.0f, 50.0f, 100.0f, 10.0f }), (Vec4f{ 1.0f, 1.0f, 1.0f, 0.0f }));
    HE_EXPECT_EQ(SmoothStep(0.0f, 1.0f, Vec4f{ 0.6f, 0.3f, 0.0f, 1.0f }), (Vec4f{ 0.648000002f, 0.216000021f, 0.0f, 1.0f }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Abs)
{
    HE_EXPECT_EQ(Abs(Vec4f{ 1, -1, 1, -1 }), (Vec4f{ 1, 1, 1, 1 }));
    HE_EXPECT_EQ(Abs(Vec4i{ 1, -1, 1, -1 }), (Vec4i{ 1, 1, 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Rcp)
{
    HE_EXPECT_EQ_ULP(Rcp(Vec4f{ 1, 2, 4, 8 }), (Vec4f{ 1.0f, 0.5f, 0.25f, 0.125f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, RcpSafe)
{
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4f{ 1, 2, 4, 8 }), (Vec4f{ 1.0f, 0.5f, 0.25f, 0.125f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4f{ Float_Min, 2, 4, 8 }), (Vec4f{ 0.0f, 0.5f, 0.25f, 0.125f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4f{ 1, Float_Min, 4, 8 }), (Vec4f{ 1.0f, 0.0f, 0.25f, 0.125f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4f{ 1, 2, Float_Min, 8 }), (Vec4f{ 1.0f, 0.5f, 0.0f, 0.125f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4f{ 1, 2, 4, Float_Min }), (Vec4f{ 1.0f, 0.5f, 0.25f, 0.0f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Sqrt)
{
    HE_EXPECT_EQ(Sqrt(Vec4f{ 1, 4, 16, 64 }), (Vec4f{ 1, 2, 4, 8 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Rsqrt)
{
    HE_EXPECT_EQ_ULP(Rsqrt(Vec4f{ 1, 4, 16, 64 }), (Vec4f{ 1.0f, 0.5f, 0.25f, 0.125f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Min)
{
    HE_EXPECT_EQ(Min(Vec4f{ 1, 2, 3, 0 }, Vec4f{ 0, 2, 4, 0 }), (Vec4f{ 0, 2, 3, 0 }));
    HE_EXPECT_EQ(Min(Vec4i{ 1, 2, 3, 0 }, Vec4i{ 0, 2, 4, 0 }), (Vec4i{ 0, 2, 3, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Max)
{
    HE_EXPECT_EQ(Max(Vec4f{ 1, 2, 3, 0 }, Vec4f{ 0, 2, 4, 0 }), (Vec4f{ 1, 2, 4, 0 }));
    HE_EXPECT_EQ(Max(Vec4i{ 1, 2, 3, 0 }, Vec4i{ 0, 2, 4, 0 }), (Vec4i{ 1, 2, 4, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Clamp)
{
    HE_EXPECT_EQ(Clamp(Vec4f{ 2, 2, 2, 2 }, Vec4f{ 1, 3, 0, 2 }, Vec4f{ 2, 4, 1, 3 }), (Vec4f{ 2, 3, 1, 2 }));
    HE_EXPECT_EQ(Clamp(Vec4i{ 2, 2, 2, 2 }, Vec4i{ 1, 3, 0, 2 }, Vec4i{ 2, 4, 1, 3 }), (Vec4i{ 2, 3, 1, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, HMin)
{
    HE_EXPECT_EQ(HMin(Vec4f{ 1, 3, 5, 7 }), 1);
    HE_EXPECT_EQ(HMin(Vec4i{ 1, 3, 5, 7 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, HMax)
{
    HE_EXPECT_EQ(HMax(Vec4f{ 1, 3, 5, 7 }), 7);
    HE_EXPECT_EQ(HMax(Vec4i{ 1, 3, 5, 7 }), 7);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, HAdd)
{
    HE_EXPECT_EQ(HAdd(Vec4f{ 1, 3, 5, 7 }), 16);
    HE_EXPECT_EQ(HAdd(Vec4i{ 1, 3, 5, 7 }), 16);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Dot)
{
    HE_EXPECT_EQ(Dot(Vec4f{ 1, 2, 3, 4 }, Vec4f{ 0, 1, 2, 3 }), 20);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, LenSquared)
{
    HE_EXPECT_EQ(LenSquared(Vec4f{ 1, 2, 3, 4 }), 30);
    HE_EXPECT_EQ(LenSquared(Vec4i{ 1, 2, 3, 4 }), 30);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Len)
{
    HE_EXPECT_EQ(Len(Vec4f{ 1, 2, 3, 4 }), Sqrt(30.f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Normalize)
{
    HE_EXPECT_EQ_ULP(Normalize(Vec4f{ 1, 1, 1, 1 }), (Vec4f{ 0.5f, 0.5f, 0.5f, 0.5f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, NormalizeSafe)
{
    HE_EXPECT_EQ_ULP(NormalizeSafe(Vec4f{ 1, 1, 1, 1 }), (Vec4f{ 0.5f, 0.5f, 0.5f, 0.5f }), 1);
    HE_EXPECT_EQ_ULP(NormalizeSafe(Vec4f{ 0, 0, 0, 0 }), (Vec4f{ 0, 0, 0, 0 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, IsNormalized)
{
    float x = Rcp(Sqrt(3.f));
    float y = Rcp(Sqrt(2.f));
    HE_EXPECT(IsNormalized(Vec4f{ 1, 0, 0, 0 }));
    HE_EXPECT(IsNormalized(Vec4f{ y, y, 0, 0 }));
    HE_EXPECT(IsNormalized(Vec4f{ x, x, x, 0 }));
    HE_EXPECT(IsNormalized(Vec4f{ 0.5f, 0.5f, 0.5f, 0.5f }));
    HE_EXPECT(!IsNormalized(Vec4f{ 1, 1, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4, Operators)
{
    {
        Vec4f v;

        HE_EXPECT_EQ((-Vec4f{ 1, 2, 3, 4 }), (Vec4f{ -1, -2, -3, -4 }));

        v = { 1, 2, 3, 4 };
        v += { 1, 2, 3, 4 };
        HE_EXPECT_EQ(v, (Vec4f{ 2, 4, 6, 8 }));
        v += 1.0f;
        HE_EXPECT_EQ(v, (Vec4f{ 3, 5, 7, 9 }));
        HE_EXPECT_EQ((Vec4f{ 1, 2, 3, 4 } + Vec4f{ 1, 2, 3, 4 }), (Vec4f{ 2, 4, 6, 8 }));
        HE_EXPECT_EQ((Vec4f{ 1, 2, 3, 4 } + 2.0f), (Vec4f{ 3, 4, 5, 6 }));

        v = { 1, 2, 3, 4 };
        v -= { 1, 2, 3, 4 };
        HE_EXPECT_EQ(v, (Vec4f{ 0, 0, 0, 0 }));
        v = { 1, 2, 3, 4 };
        v -= 1.0f;
        HE_EXPECT_EQ(v, (Vec4f{ 0, 1, 2, 3 }));
        HE_EXPECT_EQ((Vec4f{ 1, 2, 3, 4 } - Vec4f{ 1, 2, 3, 4 }), (Vec4f{ 0, 0, 0, 0 }));
        HE_EXPECT_EQ((Vec4f{ 1, 2, 3, 4 } - 2.0f), (Vec4f{ -1, 0, 1, 2 }));

        v = { 1, 2, 3, 4 };
        v *= 2.0f;
        HE_EXPECT_EQ(v, (Vec4f{ 2, 4, 6, 8 }));
        HE_EXPECT_EQ((Vec4f{ 1, 2, 3, 4 } * 2.0f), (Vec4f{ 2, 4, 6, 8 }));

        v = { 1, 2, 3, 4 };
        v *= { 1, 2, 3, 4 };
        HE_EXPECT_EQ(v, (Vec4f{ 1, 4, 9, 16 }));
        HE_EXPECT_EQ((Vec4f{ 1, 2, 3, 4 } * Vec4f{ 1, 2, 3, 4 }), (Vec4f{ 1, 4, 9, 16 }));

        v = { 2, 4, 6, 8 };
        v /= 2.0f;
        HE_EXPECT_EQ(v, (Vec4f{ 1, 2, 3, 4 }));
        v = { 2, 4, 6, 8 };
        v /= { 1, 2, 2, 4 };
        HE_EXPECT_EQ(v, (Vec4f{ 2, 2, 3, 2 }));
        HE_EXPECT_EQ_ULP((Vec4f{ 2, 4, 6, 8 } / 2.0f), (Vec4f{ 1, 2, 3, 4 }), 1);
        HE_EXPECT_EQ_ULP((Vec4f{ 4, 4, 0, 32 } / Vec4f{ 2, 4, 6, 8 }), (Vec4f{ 2, 1, 0, 4 }), 1);

        HE_EXPECT_LT((Vec4f{ 1, 1, 1, 1 }), (Vec4f{ 2, 0, 0, 0 }));
        HE_EXPECT_LT((Vec4f{ 1, 1, 1, 1 }), (Vec4f{ 1, 2, 0, 0 }));
        HE_EXPECT_LT((Vec4f{ 1, 1, 1, 1 }), (Vec4f{ 1, 1, 2, 0 }));
        HE_EXPECT_LT((Vec4f{ 1, 1, 1, 1 }), (Vec4f{ 1, 1, 1, 2 }));

        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } < Vec4f{ 0, 2, 2, 2 }));
        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } < Vec4f{ 1, 0, 2, 2 }));
        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } < Vec4f{ 1, 1, 0, 2 }));
        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } < Vec4f{ 1, 1, 1, 0 }));

        HE_EXPECT((Vec4f{ 1, 1, 1, 1 } == Vec4f{ 1, 1, 1, 1 }));
        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } == Vec4f{ 0, 1, 1, 1 }));
        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } == Vec4f{ 1, 0, 1, 1 }));
        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } == Vec4f{ 1, 1, 0, 1 }));
        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } == Vec4f{ 1, 1, 1, 0 }));

        HE_EXPECT(!(Vec4f{ 1, 1, 1, 1 } != Vec4f{ 1, 1, 1, 1 }));
        HE_EXPECT((Vec4f{ 1, 1, 1, 1 } != Vec4f{ 0, 1, 1, 1 }));
        HE_EXPECT((Vec4f{ 1, 1, 1, 1 } != Vec4f{ 1, 0, 1, 1 }));
        HE_EXPECT((Vec4f{ 1, 1, 1, 1 } != Vec4f{ 1, 1, 0, 1 }));
        HE_EXPECT((Vec4f{ 1, 1, 1, 1 } != Vec4f{ 1, 1, 1, 0 }));
    }

    {
        Vec4i v;

        HE_EXPECT_EQ((-Vec4i{ 1, 2, 3, 4 }), (Vec4i{ -1, -2, -3, -4 }));

        v = { 1, 2, 3, 4 };
        v += { 1, 2, 3, 4 };
        HE_EXPECT_EQ(v, (Vec4i{ 2, 4, 6, 8 }));
        v += 1;
        HE_EXPECT_EQ(v, (Vec4i{ 3, 5, 7, 9 }));
        HE_EXPECT_EQ((Vec4i{ 1, 2, 3, 4 } + Vec4i{ 1, 2, 3, 4 }), (Vec4i{ 2, 4, 6, 8 }));
        HE_EXPECT_EQ((Vec4i{ 1, 2, 3, 4 } + 2), (Vec4i{ 3, 4, 5, 6 }));

        v = { 1, 2, 3, 4 };
        v -= { 1, 2, 3, 4 };
        HE_EXPECT_EQ(v, (Vec4i{ 0, 0, 0, 0 }));
        v = { 1, 2, 3, 4 };
        v -= 1;
        HE_EXPECT_EQ(v, (Vec4i{ 0, 1, 2, 3 }));
        HE_EXPECT_EQ((Vec4i{ 1, 2, 3, 4 } - Vec4i{ 1, 2, 3, 4 }), (Vec4i{ 0, 0, 0, 0 }));
        HE_EXPECT_EQ((Vec4i{ 1, 2, 3, 4 } - 2), (Vec4i{ -1, 0, 1, 2 }));

        v = { 1, 2, 3, 4 };
        v *= 2;
        HE_EXPECT_EQ(v, (Vec4i{ 2, 4, 6, 8 }));
        v = { 1, 2, 3, 4 };
        v *= { 1, 2, 3, 4 };
        HE_EXPECT_EQ(v, (Vec4i{ 1, 4, 9, 16 }));
        HE_EXPECT_EQ((Vec4i{ 1, 2, 3, 4 } * 2), (Vec4i{ 2, 4, 6, 8 }));
        HE_EXPECT_EQ((Vec4i{ 1, 2, 3, 4 } * Vec4i{ 1, 2, 3, 4 }), (Vec4i{ 1, 4, 9, 16 }));

        v = { 2, 4, 6, 8 };
        v /= 2;
        HE_EXPECT_EQ(v, (Vec4i{ 1, 2, 3, 4 }));
        v = { 2, 4, 6, 8 };
        v /= { 1, 2, 2, 4 };
        HE_EXPECT_EQ(v, (Vec4i{ 2, 2, 3, 2 }));
        HE_EXPECT_EQ((Vec4i{ 2, 4, 6, 8 } / 2), (Vec4i{ 1, 2, 3, 4 }));
        HE_EXPECT_EQ((Vec4i{ 4, 4, 0, 32 } / Vec4i{ 2, 4, 6, 8 }), (Vec4i{ 2, 1, 0, 4 }));

        HE_EXPECT_LT((Vec4i{ 1, 1, 1, 1 }), (Vec4i{ 2, 0, 0, 0 }));
        HE_EXPECT_LT((Vec4i{ 1, 1, 1, 1 }), (Vec4i{ 1, 2, 0, 0 }));
        HE_EXPECT_LT((Vec4i{ 1, 1, 1, 1 }), (Vec4i{ 1, 1, 2, 0 }));
        HE_EXPECT_LT((Vec4i{ 1, 1, 1, 1 }), (Vec4i{ 1, 1, 1, 2 }));

        HE_EXPECT(!(Vec4i{ 1, 1, 1, 1 } < Vec4i{ 0, 2, 2, 2 }));
        HE_EXPECT(!(Vec4i{ 1, 1, 1, 1 } < Vec4i{ 1, 0, 2, 2 }));
        HE_EXPECT(!(Vec4i{ 1, 1, 1, 1 } < Vec4i{ 1, 1, 0, 2 }));
        HE_EXPECT(!(Vec4i{ 1, 1, 1, 1 } < Vec4i{ 1, 1, 1, 0 }));

        HE_EXPECT((Vec4i{ 1, 1, 1, 1 } == Vec4i{ 1, 1, 1, 1 }));
        HE_EXPECT(!(Vec4i{ 1, 1, 1, 1 } == Vec4i{ 0, 1, 1, 1 }));
        HE_EXPECT(!(Vec4i{ 1, 1, 1, 1 } == Vec4i{ 1, 0, 1, 1 }));
        HE_EXPECT(!(Vec4i{ 1, 1, 1, 1 } == Vec4i{ 1, 1, 0, 1 }));
        HE_EXPECT(!(Vec4i{ 1, 1, 1, 1 } == Vec4i{ 1, 1, 1, 0 }));
    }
}
