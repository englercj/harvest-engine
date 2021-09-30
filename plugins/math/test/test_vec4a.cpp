// Copyright Chad Engler

#include "vec_ulp_diff.h"

#include "he/math/vec4a.h"

#include "he/core/test.h"
#include "he/math/types_fmt.h"

using namespace he;

#define HE_EXPECT_EQ3(a, b) HE_EXPECT(All3(Eq((a), (b))), a, b)
#define HE_EXPECT_EQ_INT(a, b) HE_EXPECT(All(EqInt((a), (b))), a, b)

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, MakeVec4a)
{
    Vec4a iota2{ 1, 2, 0, 0 };
    Vec4a iota3{ 1, 2, 3, 0 };
    Vec4a iota4{ 1, 2, 3, 4 };
    Vec4a one{ 1, 1, 1, 1 };
    Vec4a two{ 2, 2, 2, 2 };
    Vec4a three{ 3, 3, 3, 3 };
    Vec4a four{ 4, 4, 4, 4 };

    HE_EXPECT_EQ(MakeVec4a(Vec2f{ 1, 2 }), iota2);
    HE_EXPECT_EQ(MakeVec4a(Vec3f{ 1, 2, 3 }), iota3);
    HE_EXPECT_EQ(MakeVec4a(Vec4f{ 1, 2, 3, 4 }), iota4);
    HE_EXPECT_EQ(MakeVec4a(Vec2i{ 1, 2 }), iota2);
    HE_EXPECT_EQ(MakeVec4a(Vec3i{ 1, 2, 3 }), iota3);
    HE_EXPECT_EQ(MakeVec4a(Vec4i{ 1, 2, 3, 4 }), iota4);
    HE_EXPECT_EQ(MakeVec4a(one, two, three), iota3);
    HE_EXPECT_EQ(MakeVec4a(one, two, three, four), iota4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, MakeVec2)
{
    HE_EXPECT_EQ(MakeVec2<float>(Vec4a{ 1, 2, 3, 4 }), (Vec2f{ 1, 2 }));
    HE_EXPECT_EQ(MakeVec2<int32_t>(Vec4a{ 1, 2, 3, 4 }), (Vec2i{ 1, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, MakeVec3)
{
    HE_EXPECT_EQ(MakeVec3<float>(Vec4a{ 1, 2, 3, 4 }), (Vec3f{ 1, 2, 3 }));
    HE_EXPECT_EQ(MakeVec3<int32_t>(Vec4a{ 1, 2, 3, 4 }), (Vec3i{ 1, 2, 3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, MakeVec4)
{
    HE_EXPECT_EQ(MakeVec4<float>(Vec4a{ 1, 2, 3, 4 }), (Vec4f{ 1, 2, 3, 4 }));
    HE_EXPECT_EQ(MakeVec4<int32_t>(Vec4a{ 1, 2, 3, 4 }), (Vec4i{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SplatZero)
{
    Vec4a zero{ 0, 0, 0, 0 };
    HE_EXPECT_EQ(SplatZero(), zero);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Splat)
{
    HE_EXPECT_EQ(Splat(1), (Vec4a{ 1, 1, 1, 1 }));
    HE_EXPECT_EQ(Splat(25), (Vec4a{ 25, 25, 25, 25 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SplatX)
{
    Vec4a iota{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(SplatX(iota), (Vec4a{ 1, 1, 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SplatY)
{
    Vec4a iota{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(SplatY(iota), (Vec4a{ 2, 2, 2, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SplatZ)
{
    Vec4a iota{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(SplatZ(iota), (Vec4a{ 3, 3, 3, 3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SplatW)
{
    Vec4a iota{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(SplatW(iota), (Vec4a{ 4, 4, 4, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, GetComponent)
{
    HE_EXPECT_EQ(GetComponent(Vec4a{ 1, 2, 3, 4 }, 0), 1);
    HE_EXPECT_EQ(GetComponent(Vec4a{ 1, 2, 3, 4 }, 1), 2);
    HE_EXPECT_EQ(GetComponent(Vec4a{ 1, 2, 3, 4 }, 2), 3);
    HE_EXPECT_EQ(GetComponent(Vec4a{ 1, 2, 3, 4 }, 3), 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SetComponent)
{
    HE_EXPECT_EQ(SetComponent(Vec4a{ 1, 2, 3, 4 }, 0, 0), (Vec4a{ 0, 2, 3, 4 }));
    HE_EXPECT_EQ(SetComponent(Vec4a{ 1, 2, 3, 4 }, 1, 0), (Vec4a{ 1, 0, 3, 4 }));
    HE_EXPECT_EQ(SetComponent(Vec4a{ 1, 2, 3, 4 }, 2, 0), (Vec4a{ 1, 2, 0, 4 }));
    HE_EXPECT_EQ(SetComponent(Vec4a{ 1, 2, 3, 4 }, 3, 0), (Vec4a{ 1, 2, 3, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, GetX)
{
    HE_EXPECT_EQ(GetX(Vec4a{ 1, 2, 3, 4 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, GetY)
{
    HE_EXPECT_EQ(GetY(Vec4a{ 1, 2, 3, 4 }), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, GetZ)
{
    HE_EXPECT_EQ(GetZ(Vec4a{ 1, 2, 3, 4 }), 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, GetW)
{
    HE_EXPECT_EQ(GetW(Vec4a{ 1, 2, 3, 4 }), 4);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SetX)
{
    HE_EXPECT_EQ(SetX(Vec4a{ 1, 2, 3, 4 }, 0), (Vec4a{ 0, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SetY)
{
    HE_EXPECT_EQ(SetY(Vec4a{ 1, 2, 3, 4 }, 0), (Vec4a{ 1, 0, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SetZ)
{
    HE_EXPECT_EQ(SetZ(Vec4a{ 1, 2, 3, 4 }, 0), (Vec4a{ 1, 2, 0, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SetW)
{
    HE_EXPECT_EQ(SetW(Vec4a{ 1, 2, 3, 4 }, 0), (Vec4a{ 1, 2, 3, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Load)
{
    HE_ALIGNED(16) float p[]{ 1, 2, 3, 4, 5 };

    HE_EXPECT_EQ(Load(p), (Vec4a{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, LoadU)
{
    HE_ALIGNED(16) float p[]{ 1, 2, 3, 4, 5 };

    HE_EXPECT_EQ(LoadU(p + 1), (Vec4a{ 2, 3, 4, 5 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Store)
{
    HE_ALIGNED(16) float p[]{ 1, 2, 3, 4, 5 };

    Vec4a x{ 3, 1, 4, 1 };
    Store(p, x);
    HE_EXPECT_EQ(p[0], 3);
    HE_EXPECT_EQ(p[1], 1);
    HE_EXPECT_EQ(p[2], 4);
    HE_EXPECT_EQ(p[3], 1);
    HE_EXPECT_EQ(p[4], 5);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, StoreU)
{
    HE_ALIGNED(16) float p[]{ 1, 2, 3, 4, 5 };

    Vec4a x{ 3, 1, 4, 1 };
    StoreU(p + 1, x);
    HE_EXPECT_EQ(p[0], 1);
    HE_EXPECT_EQ(p[1], 3);
    HE_EXPECT_EQ(p[2], 1);
    HE_EXPECT_EQ(p[3], 4);
    HE_EXPECT_EQ(p[4], 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsNan)
{
    HE_EXPECT(IsNan(Vec4a{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Vec4a{ Float_Nan, Float_Nan, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Vec4a{ Float_Nan, Float_Nan, 1, Float_Nan }));
    HE_EXPECT(IsNan(Vec4a{ Float_Nan, 1, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Vec4a{ 1, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Vec4a{ Float_Nan, 1, 1, 1 }));
    HE_EXPECT(IsNan(Vec4a{ 1, Float_Nan, 1, 1 }));
    HE_EXPECT(IsNan(Vec4a{ 1, 1, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Vec4a{ 1, 1, 1, Float_Nan }));
    HE_EXPECT(!IsNan(Vec4a{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(!IsNan(Vec4a{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsNan3)
{
    HE_EXPECT(IsNan3(Vec4a{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan3(Vec4a{ Float_Nan, Float_Nan, Float_Nan, 1 }));
    HE_EXPECT(IsNan3(Vec4a{ Float_Nan, Float_Nan, 1, Float_Nan }));
    HE_EXPECT(IsNan3(Vec4a{ Float_Nan, 1, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan3(Vec4a{ 1, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan3(Vec4a{ Float_Nan, 1, 1, 1 }));
    HE_EXPECT(IsNan3(Vec4a{ 1, Float_Nan, 1, 1 }));
    HE_EXPECT(IsNan3(Vec4a{ 1, 1, Float_Nan, 1 }));
    HE_EXPECT(!IsNan3(Vec4a{ 1, 1, 1, Float_Nan }));
    HE_EXPECT(!IsNan3(Vec4a{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(!IsNan3(Vec4a{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsInfinite)
{
    HE_EXPECT(IsInfinite(Vec4a{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec4a{ Float_Infinity, Float_Infinity, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Vec4a{ Float_Infinity, Float_Infinity, 1, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec4a{ Float_Infinity, 1, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec4a{ 1, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Vec4a{ Float_Infinity, 1, 1, 1 }));
    HE_EXPECT(IsInfinite(Vec4a{ 1, Float_Infinity, 1, 1 }));
    HE_EXPECT(IsInfinite(Vec4a{ 1, 1, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Vec4a{ 1, 1, 1, Float_Infinity }));
    HE_EXPECT(!IsInfinite(Vec4a{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(!IsInfinite(Vec4a{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsInfinite3)
{
    HE_EXPECT(IsInfinite3(Vec4a{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite3(Vec4a{ Float_Infinity, Float_Infinity, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite3(Vec4a{ Float_Infinity, Float_Infinity, 1, Float_Infinity }));
    HE_EXPECT(IsInfinite3(Vec4a{ Float_Infinity, 1, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite3(Vec4a{ 1, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite3(Vec4a{ Float_Infinity, 1, 1, 1 }));
    HE_EXPECT(IsInfinite3(Vec4a{ 1, Float_Infinity, 1, 1 }));
    HE_EXPECT(IsInfinite3(Vec4a{ 1, 1, Float_Infinity, 1 }));
    HE_EXPECT(!IsInfinite3(Vec4a{ 1, 1, 1, Float_Infinity }));
    HE_EXPECT(!IsInfinite3(Vec4a{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(!IsInfinite3(Vec4a{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsFinite)
{
    HE_EXPECT(IsFinite(Vec4a{ 1, 2, 3, 4 }));
    HE_EXPECT(!IsFinite(Vec4a{ Float_Infinity, 2, 3, 4 }));
    HE_EXPECT(!IsFinite(Vec4a{ 1, Float_Infinity, 3, 4 }));
    HE_EXPECT(!IsFinite(Vec4a{ 1, 2, Float_Infinity, 4 }));
    HE_EXPECT(!IsFinite(Vec4a{ 1, 2, 3, Float_Infinity }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsFinite3)
{
    HE_EXPECT(IsFinite3(Vec4a{ 1, 2, 3, 4 }));
    HE_EXPECT(!IsFinite3(Vec4a{ Float_Infinity, 2, 3, 4 }));
    HE_EXPECT(!IsFinite3(Vec4a{ 1, Float_Infinity, 3, 4 }));
    HE_EXPECT(!IsFinite3(Vec4a{ 1, 2, Float_Infinity, 4 }));
    HE_EXPECT(IsFinite3(Vec4a{ 1, 2, 3, Float_Infinity }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsZeroSafe)
{
    HE_EXPECT(IsZeroSafe(Vec4a{ 1, 2, 3, 4 }));
    HE_EXPECT(IsZeroSafe(Vec4a{ 0, 0, 0, 4 }));
    HE_EXPECT(!IsZeroSafe(Vec4a{ 0, 0, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsZeroSafe3)
{
    HE_EXPECT(IsZeroSafe3(Vec4a{ 1, 2, 3, 4 }));
    HE_EXPECT(!IsZeroSafe3(Vec4a{ 0, 0, 0, 4 }));
    HE_EXPECT(!IsZeroSafe3(Vec4a{ 0, 0, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Negate)
{
    HE_EXPECT_EQ(Negate(Vec4a{ 1, 2, 3, 4 }), (Vec4a{ -1, -2, -3, -4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Add)
{
    HE_EXPECT_EQ(Add(Vec4a{ 1, 2, 3, 4 }, Vec4a{ 1, 2, 3, 4 }), (Vec4a{ 2, 4, 6, 8 }));
    HE_EXPECT_EQ(Add(Vec4a{ 1, 2, 3, 4 }, 2.0f), (Vec4a{ 3, 4, 5, 6 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Sub)
{
    HE_EXPECT_EQ(Sub(Vec4a{ 1, 2, 3, 4 }, Vec4a{ 1, 2, 3, 4 }), (Vec4a{ 0, 0, 0, 0 }));
    HE_EXPECT_EQ(Sub(Vec4a{ 1, 2, 3, 4 }, 1.0f), (Vec4a{ 0, 1, 2, 3 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Mul)
{
    HE_EXPECT_EQ(Mul(Vec4a{ 1, 2, 3, 4 }, Vec4a{ 1, 2, 3, 4 }), (Vec4a{ 1, 4, 9, 16 }));
    HE_EXPECT_EQ(Mul(Vec4a{ 1, 2, 3, 4 }, 2), (Vec4a{ 2, 4, 6, 8 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, MulAdd)
{
    HE_EXPECT_EQ(MulAdd(Vec4a{ 1, 2, 3, 4 }, Vec4a{ 1, 2, 3, 4 }, Vec4a{ 1, 2, 3, 4 }), (Vec4a{ 2, 6, 12, 20 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Div)
{
    HE_EXPECT_EQ_ULP(Div(Vec4a{ 1, 2, 3, 4 }, Vec4a{ 1, 2, 3, 4 }), (Vec4a{ 1, 1, 1, 1 }), 1);
    HE_EXPECT_EQ_ULP(Div(Vec4a{ 1, 2, 3, 4 }, 2.0f), (Vec4a{ 0.5f, 1.0f, 1.5f, 2.0f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Lerp)
{
    HE_EXPECT_EQ(Lerp(Vec4a{ 1, 1, 1, 1 }, Vec4a{ 2, 3, 5, 1 }, 0.0f), (Vec4a{ 1.00f, 1.0f, 1.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec4a{ 1, 1, 1, 1 }, Vec4a{ 2, 3, 5, 1 }, 0.25f), (Vec4a{ 1.25f, 1.5f, 2.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec4a{ 1, 1, 1, 1 }, Vec4a{ 2, 3, 5, 1 }, 0.5f), (Vec4a{ 1.50f, 2.0f, 3.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec4a{ 1, 1, 1, 1 }, Vec4a{ 2, 3, 5, 1 }, 0.75f), (Vec4a{ 1.75f, 2.5f, 4.0f, 1.0f }));
    HE_EXPECT_EQ(Lerp(Vec4a{ 1, 1, 1, 1 }, Vec4a{ 2, 3, 5, 1 }, 1.0f), (Vec4a{ 2.00f, 3.0f, 5.0f, 1.0f }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, SmoothStep)
{
    HE_EXPECT_EQ(SmoothStep(Splat(100.0f), Splat(200.0f), Vec4a{ 150.0f, 200.0f, 300.0f, 100.0f }), (Vec4a{ 0.5f, 1.0f, 1.0f, 0.0f }));
    HE_EXPECT_EQ_ULP(SmoothStep(Splat(10.0f), Splat(20.0f), Vec4a{ 9.0f, 15.0f, 20.0f, 10.0f }), (Vec4a{ 0.0f, 0.5f, 1.0f, 0.0f }), 2);
    HE_EXPECT_EQ(SmoothStep(Splat(10.0f), Splat(20.0f), Vec4a{ 30.0f, 50.0f, 100.0f, 10.0f }), (Vec4a{ 1.0f, 1.0f, 1.0f, 0.0f }));
    HE_EXPECT_EQ_ULP(SmoothStep(Splat(0.0f), Splat(1.0f), Vec4a{ 0.6f, 0.3f, 0.0f, 1.0f }), (Vec4a{ 0.648f, 0.216f, 0.0f, 1.0f }), 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Abs)
{
    HE_EXPECT_EQ(Abs(Vec4a{ 1, -1, 1, -1 }), (Vec4a{ 1, 1, 1, 1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Rcp)
{
    HE_EXPECT_EQ_ULP(Rcp(Vec4a{ 1, 2, 4, 8 }), (Vec4a{ 1.0f, 0.5f, 0.25f, 0.125f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, RcpSafe)
{
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4a{ 1, 2, 4, 8 }), (Vec4a{ 1.0f, 0.5f, 0.25f, 0.125f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4a{ Float_Min, 2, 4, 8 }), (Vec4a{ 0.0f, 0.5f, 0.25f, 0.125f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4a{ 1, Float_Min, 4, 8 }), (Vec4a{ 1.0f, 0.0f, 0.25f, 0.125f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4a{ 1, 2, Float_Min, 8 }), (Vec4a{ 1.0f, 0.5f, 0.0f, 0.125f }), 1);
    HE_EXPECT_EQ_ULP(RcpSafe(Vec4a{ 1, 2, 4, Float_Min }), (Vec4a{ 1.0f, 0.5f, 0.25f, 0.0f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Sqrt)
{
    HE_EXPECT_EQ_ULP(Sqrt(Vec4a{ 1, 4, 16, 64 }), (Vec4a{ 1, 2, 4, 8 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Rsqrt)
{
    HE_EXPECT_EQ_ULP(Rsqrt(Vec4a{ 1, 4, 16, 64 }), (Vec4a{ 1.0f, 0.5f, 0.25f, 0.125f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Min)
{
    HE_EXPECT_EQ(Min(Vec4a{ 1, 2, 3, 0 }, Vec4a{ 0, 2, 4, 0 }), (Vec4a{ 0, 2, 3, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Max)
{
    HE_EXPECT_EQ(Max(Vec4a{ 1, 2, 3, 0 }, Vec4a{ 0, 2, 4, 0 }), (Vec4a{ 1, 2, 4, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Clamp)
{
    HE_EXPECT_EQ(Clamp(Vec4a{ 2, 2, 2, 2 }, Vec4a{ 1, 3, 0, 2 }, Vec4a{ 2, 4, 1, 3 }), (Vec4a{ 2, 3, 1, 2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, HMin)
{
    HE_EXPECT_EQ(HMin(Vec4a{ 1, 3, 5, 7 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, HMax)
{
    HE_EXPECT_EQ(HMax(Vec4a{ 1, 3, 5, 7 }), 7);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, HAdd)
{
    HE_EXPECT_EQ(HAdd(Vec4a{ 1, 3, 5, 7 }), 16);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Dot)
{
    HE_EXPECT_EQ(Dot(Vec4a{ 1, 2, 3, 4 }, Vec4a{ 0, 1, 2, 3 }), 20);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Dot3)
{
    HE_EXPECT_EQ(Dot3(Vec4a{ 1, 2, 3, 4 }, Vec4a{ 0, 1, 2, 3 }), 8);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Cross)
{
    HE_EXPECT_EQ(Cross(Vec4a{ 1, 0, 0, 5 }, Vec4a{ 0, 1, 0, 7 }), (Vec4a{ 0, 0, 1, 0 }));
    HE_EXPECT_EQ(Cross(Vec4a{ 0, 1, 0, 5 }, Vec4a{ 0, 0, 1, 7 }), (Vec4a{ 1, 0, 0, 0 }));
    HE_EXPECT_EQ(Cross(Vec4a{ 0, 0, 1, 5 }, Vec4a{ 1, 0, 0, 7 }), (Vec4a{ 0, 1, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, LenSquared)
{
    HE_EXPECT_EQ(LenSquared(Vec4a{ 1, 2, 3, 4 }), 30);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, LenSquared3)
{
    HE_EXPECT_EQ(LenSquared3(Vec4a{ 1, 2, 3, 4 }), 14);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Len)
{
    HE_EXPECT_EQ(Len(Vec4a{ 1, 2, 3, 4 }), Sqrt(30.f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Len3)
{
    HE_EXPECT_EQ(Len3(Vec4a{ 1, 2, 3, 4 }), Sqrt(14.f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Normalize)
{
    HE_EXPECT_EQ_ULP(Normalize(Vec4a{ 1, 1, 1, 1 }), (Vec4a{ 0.5f, 0.5f, 0.5f, 0.5f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Normalize3)
{
    const float x = Rcp(Sqrt(3.f));
    HE_EXPECT_EQ3(Normalize3(Vec4a{ 1, 1, 1, 1 }), (Vec4a{ x, x, x, x }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsNormalized)
{
    const float x = Rcp(Sqrt(3.f));
    const float y = Rcp(Sqrt(2.f));

    HE_EXPECT(IsNormalized(Vec4a{ 1, 0, 0, 0 }));
    HE_EXPECT(IsNormalized(Vec4a{ y, y, 0, 0 }));
    HE_EXPECT(IsNormalized(Vec4a{ x, x, x, 0 }));
    HE_EXPECT(IsNormalized(Vec4a{ 0.5f, 0.5f, 0.5f, 0.5f }));
    HE_EXPECT(!IsNormalized(Vec4a{ 1, 1, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, IsNormalized3)
{
    const float x = Rcp(Sqrt(3.f));
    const float y = Rcp(Sqrt(2.f));

    HE_EXPECT(IsNormalized3(Vec4a{ 1, 0, 0, 0 }));
    HE_EXPECT(IsNormalized3(Vec4a{ y, y, 0, 0 }));
    HE_EXPECT(IsNormalized3(Vec4a{ x, x, x, 0 }));
    HE_EXPECT(IsNormalized3(Vec4a{ 1, 0, 0, 1 }));
    HE_EXPECT(!IsNormalized3(Vec4a{ 1, 1, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Lt)
{
    Vec4a a = Vec4a{ 1, 2, 3, 1 };
    Vec4a b = Vec4a{ 2, 2, 2, 2 };

    HE_EXPECT_EQ_INT(Lt(a, b), (Vec4a{ Float_AllBits, 0, 0, Float_AllBits }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Le)
{
    Vec4a a = Vec4a{ 1, 2, 3, 1 };
    Vec4a b = Vec4a{ 2, 2, 2, 2 };

    HE_EXPECT_EQ_INT(Le(a, b), (Vec4a{ Float_AllBits, Float_AllBits, 0, Float_AllBits }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Gt)
{
    Vec4a a = Vec4a{ 1, 2, 3, 1 };
    Vec4a b = Vec4a{ 2, 2, 2, 2 };

    HE_EXPECT_EQ_INT(Gt(a, b), (Vec4a{ 0, 0, Float_AllBits, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Ge)
{
    Vec4a a = Vec4a{ 1, 2, 3, 1 };
    Vec4a b = Vec4a{ 2, 2, 2, 2 };

    HE_EXPECT_EQ_INT(Ge(a, b), (Vec4a{ 0, Float_AllBits, Float_AllBits, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Eq)
{
    Vec4a a = Vec4a{ 1, 2, 3, 1 };
    Vec4a b = Vec4a{ 2, 2, 2, 2 };

    HE_EXPECT_EQ_INT(Eq(a, b), (Vec4a{ 0, Float_AllBits, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Ne)
{
    Vec4a a = Vec4a{ 1, 2, 3, 1 };
    Vec4a b = Vec4a{ 2, 2, 2, 2 };

    HE_EXPECT_EQ_INT(Ne(a, b), (Vec4a{ Float_AllBits, 0, Float_AllBits, Float_AllBits }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Any)
{
    Vec4a v;

    v = Vec4a{ Float_AllBits, Float_AllBits, Float_AllBits, Float_AllBits };
    HE_EXPECT(Any(v));

    v = Vec4a{ Float_AllBits, Float_AllBits, Float_AllBits, 0 };
    HE_EXPECT(Any(v));

    v = Vec4a{ Float_AllBits, Float_AllBits, 0, 0 };
    HE_EXPECT(Any(v));

    v = Vec4a{ 0, 0, 0, Float_AllBits };
    HE_EXPECT(Any(v));

    v = Vec4a{ 0, 0, 0, 0 };
    HE_EXPECT(!Any(v));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Any3)
{
    Vec4a v;

    v = Vec4a{ Float_AllBits, Float_AllBits, Float_AllBits, Float_AllBits };
    HE_EXPECT(Any3(v));

    v = Vec4a{ Float_AllBits, Float_AllBits, Float_AllBits, 0 };
    HE_EXPECT(Any3(v));

    v = Vec4a{ Float_AllBits, Float_AllBits, 0, 0 };
    HE_EXPECT(Any3(v));

    v = Vec4a{ 0, 0, 0, Float_AllBits };
    HE_EXPECT(!Any3(v));

    v = Vec4a{ 0, 0, 0, 0 };
    HE_EXPECT(!Any3(v));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, All)
{
    Vec4a v;

    v = Vec4a{ Float_AllBits, Float_AllBits, Float_AllBits, Float_AllBits };
    HE_EXPECT(All(v));

    v = Vec4a{ Float_AllBits, Float_AllBits, Float_AllBits, 0 };
    HE_EXPECT(!All(v));

    v = Vec4a{ Float_AllBits, Float_AllBits, 0, 0 };
    HE_EXPECT(!All(v));

    v = Vec4a{ 0, 0, 0, Float_AllBits };
    HE_EXPECT(!All(v));

    v = Vec4a{ 0, 0, 0, 0 };
    HE_EXPECT(!All(v));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, All3)
{
    Vec4a v;

    v = Vec4a{ Float_AllBits, Float_AllBits, Float_AllBits, Float_AllBits };
    HE_EXPECT(All3(v));

    v = Vec4a{ Float_AllBits, Float_AllBits, Float_AllBits, 0 };
    HE_EXPECT(All3(v));

    v = Vec4a{ Float_AllBits, Float_AllBits, 0, 0 };
    HE_EXPECT(!All3(v));

    v = Vec4a{ 0, 0, 0, Float_AllBits };
    HE_EXPECT(!All3(v));

    v = Vec4a{ 0, 0, 0, 0 };
    HE_EXPECT(!All3(v));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, vec4a, Operators)
{
    Vec4a v;

    HE_EXPECT_EQ((-Vec4a{ 1, 2, 3, 4 }), (Vec4a{ -1, -2, -3, -4 }));

    v = Vec4a{ 1, 2, 3, 4 };
    v += Vec4a{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(v, (Vec4a{ 2, 4, 6, 8 }));
    HE_EXPECT_EQ((Vec4a{ 1, 2, 3, 4 } + Vec4a{ 1, 2, 3, 4 }), (Vec4a{ 2, 4, 6, 8 }));

    v = Vec4a{ 1, 2, 3, 4 };
    v -= Vec4a{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(v, (Vec4a{ 0, 0, 0, 0 }));
    HE_EXPECT_EQ((Vec4a{ 1, 2, 3, 4 } - Vec4a{ 1, 2, 3, 4 }), (Vec4a{ 0, 0, 0, 0 }));

    v = Vec4a{ 1, 2, 3, 4 };
    v *= Vec4a{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(v, (Vec4a{ 1, 4, 9, 16 }));
    HE_EXPECT_EQ((Vec4a{ 1, 2, 3, 4 } * Vec4a{ 1, 2, 3, 4 }), (Vec4a{ 1, 4, 9, 16 }));

    // Some platforms generate small numerical errors on division
    v = Vec4a{ 2, 4, 6, 8 };
    v /= Vec4a{ 1, 2, 2, 4 };
    HE_EXPECT_EQ_ULP(v, (Vec4a{ 2, 2, 3, 2 }), 1);
    v = Vec4a{ 4, 4, 0, 32 } / Vec4a{ 2, 4, 6, 8 };
    HE_EXPECT_EQ_ULP(v, (Vec4a{ 2, 1, 0, 4 }), 1);

    HE_EXPECT_LT((Vec4a{ 1, 1, 1, 1 }), (Vec4a{ 2, 0, 0, 0 }));
    HE_EXPECT_LT((Vec4a{ 1, 1, 1, 1 }), (Vec4a{ 1, 2, 0, 0 }));
    HE_EXPECT_LT((Vec4a{ 1, 1, 1, 1 }), (Vec4a{ 1, 1, 2, 0 }));
    HE_EXPECT_LT((Vec4a{ 1, 1, 1, 1 }), (Vec4a{ 1, 1, 1, 2 }));

    HE_EXPECT(!(Vec4a{ 1, 1, 1, 1 } < Vec4a{ 0, 2, 2, 2 }));
    HE_EXPECT(!(Vec4a{ 1, 1, 1, 1 } < Vec4a{ 1, 0, 2, 2 }));
    HE_EXPECT(!(Vec4a{ 1, 1, 1, 1 } < Vec4a{ 1, 1, 0, 2 }));
    HE_EXPECT(!(Vec4a{ 1, 1, 1, 1 } < Vec4a{ 1, 1, 1, 0 }));

    HE_EXPECT((Vec4a{ 1, 1, 1, 1 } == Vec4a{ 1, 1, 1, 1 }));
    HE_EXPECT(!(Vec4a{ 1, 1, 1, 1 } == Vec4a{ 0, 1, 1, 1 }));
    HE_EXPECT(!(Vec4a{ 1, 1, 1, 1 } == Vec4a{ 1, 0, 1, 1 }));
    HE_EXPECT(!(Vec4a{ 1, 1, 1, 1 } == Vec4a{ 1, 1, 0, 1 }));
    HE_EXPECT(!(Vec4a{ 1, 1, 1, 1 } == Vec4a{ 1, 1, 1, 0 }));
}
