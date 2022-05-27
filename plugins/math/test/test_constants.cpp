// Copyright Chad Engler

#include "vec_ulp_diff.h"

#include "he/math/constants.h"
#include "he/math/mat44.h"
#include "he/math/quata.h"
#include "he/math/types_fmt.h"
#include "he/math/vec4a.h"

#include "he/core/test.h"

#include <cfloat>
#include <cmath>

using namespace he;

#define HE_EXPECT_EQ_VEC4A(a, b) HE_EXPECT(All((a) == (b)))

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Float)
{
    HE_EXPECT_EQ(cosf(Float_Pi), -1.0f);
    HE_EXPECT_EQ(acosf(-1.0f), Float_Pi);

    HE_EXPECT_EQ(cosf(Float_Pi2), 1.0f);
    HE_EXPECT_EQ(Float_Pi2, 2.0f * Float_Pi);

    HE_EXPECT_EQ(Float_PiHalf, Float_Pi / 2.0f);
    HE_EXPECT_EQ(Float_PiQuarter, Float_Pi / 4.0f);
    HE_EXPECT_EQ(Float_Sqrt2, sqrtf(2.0f));

    HE_EXPECT_EQ(Float_Epsilon, FLT_EPSILON);
    HE_EXPECT_EQ(Float_Min, FLT_MIN);
    HE_EXPECT_EQ(Float_Max, FLT_MAX);
    HE_EXPECT_EQ(BitCast<uint32_t>(Float_AllBits), 0xffffffff);
    HE_EXPECT_EQ(BitCast<uint32_t>(Float_Infinity), 0x7f800000);
    HE_EXPECT_EQ(Float_ZeroSafe, FLT_MIN * 1000.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Vec4a)
{
    HE_EXPECT_EQ(GetX(Vec4a_Infinity), Float_Infinity);
    HE_EXPECT_EQ(GetY(Vec4a_Infinity), Float_Infinity);
    HE_EXPECT_EQ(GetZ(Vec4a_Infinity), Float_Infinity);
    HE_EXPECT_EQ(GetW(Vec4a_Infinity), Float_Infinity);
    HE_EXPECT_EQ(GetX(Vec4a_Zero), 0.0f);
    HE_EXPECT_EQ(GetY(Vec4a_Zero), 0.0f);
    HE_EXPECT_EQ(GetZ(Vec4a_Zero), 0.0f);
    HE_EXPECT_EQ(GetW(Vec4a_Zero), 0.0f);
    HE_EXPECT_EQ(GetX(Vec4a_One), 1.0f);
    HE_EXPECT_EQ(GetY(Vec4a_One), 1.0f);
    HE_EXPECT_EQ(GetZ(Vec4a_One), 1.0f);
    HE_EXPECT_EQ(GetW(Vec4a_One), 1.0f);
    HE_EXPECT_EQ(GetX(Vec4a_X), 1.0f);
    HE_EXPECT_EQ(GetY(Vec4a_X), 0.0f);
    HE_EXPECT_EQ(GetZ(Vec4a_X), 0.0f);
    HE_EXPECT_EQ(GetW(Vec4a_X), 0.0f);
    HE_EXPECT_EQ(GetX(Vec4a_Y), 0.0f);
    HE_EXPECT_EQ(GetY(Vec4a_Y), 1.0f);
    HE_EXPECT_EQ(GetZ(Vec4a_Y), 0.0f);
    HE_EXPECT_EQ(GetW(Vec4a_Y), 0.0f);
    HE_EXPECT_EQ(GetX(Vec4a_Z), 0.0f);
    HE_EXPECT_EQ(GetY(Vec4a_Z), 0.0f);
    HE_EXPECT_EQ(GetZ(Vec4a_Z), 1.0f);
    HE_EXPECT_EQ(GetW(Vec4a_Z), 0.0f);
    HE_EXPECT_EQ(GetX(Vec4a_W), 0.0f);
    HE_EXPECT_EQ(GetY(Vec4a_W), 0.0f);
    HE_EXPECT_EQ(GetZ(Vec4a_W), 0.0f);
    HE_EXPECT_EQ(GetW(Vec4a_W), 1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Vec2f)
{
    HE_EXPECT_EQ(Vec2f_Infinity.x, Float_Infinity);
    HE_EXPECT_EQ(Vec2f_Infinity.y, Float_Infinity);
    HE_EXPECT_EQ(Vec2f_Zero.x, 0.0f);
    HE_EXPECT_EQ(Vec2f_Zero.y, 0.0f);
    HE_EXPECT_EQ(Vec2f_One.x, 1.0f);
    HE_EXPECT_EQ(Vec2f_One.y, 1.0f);
    HE_EXPECT_EQ(Vec2f_X.x, 1.0f);
    HE_EXPECT_EQ(Vec2f_X.y, 0.0f);
    HE_EXPECT_EQ(Vec2f_Y.x, 0.0f);
    HE_EXPECT_EQ(Vec2f_Y.y, 1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Vec3f)
{
    HE_EXPECT_EQ(Vec3f_Infinity.x, Float_Infinity);
    HE_EXPECT_EQ(Vec3f_Infinity.y, Float_Infinity);
    HE_EXPECT_EQ(Vec3f_Infinity.z, Float_Infinity);
    HE_EXPECT_EQ(Vec3f_Zero.x, 0.0f);
    HE_EXPECT_EQ(Vec3f_Zero.y, 0.0f);
    HE_EXPECT_EQ(Vec3f_Zero.z, 0.0f);
    HE_EXPECT_EQ(Vec3f_One.x, 1.0f);
    HE_EXPECT_EQ(Vec3f_One.y, 1.0f);
    HE_EXPECT_EQ(Vec3f_One.z, 1.0f);
    HE_EXPECT_EQ(Vec3f_X.x, 1.0f);
    HE_EXPECT_EQ(Vec3f_X.y, 0.0f);
    HE_EXPECT_EQ(Vec3f_X.z, 0.0f);
    HE_EXPECT_EQ(Vec3f_Y.x, 0.0f);
    HE_EXPECT_EQ(Vec3f_Y.y, 1.0f);
    HE_EXPECT_EQ(Vec3f_Y.z, 0.0f);
    HE_EXPECT_EQ(Vec3f_Z.x, 0.0f);
    HE_EXPECT_EQ(Vec3f_Z.y, 0.0f);
    HE_EXPECT_EQ(Vec3f_Z.z, 1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Vec4f)
{
    HE_EXPECT_EQ(Vec4f_Infinity.x, Float_Infinity);
    HE_EXPECT_EQ(Vec4f_Infinity.y, Float_Infinity);
    HE_EXPECT_EQ(Vec4f_Infinity.z, Float_Infinity);
    HE_EXPECT_EQ(Vec4f_Infinity.w, Float_Infinity);
    HE_EXPECT_EQ(Vec4f_Zero.x, 0.0f);
    HE_EXPECT_EQ(Vec4f_Zero.y, 0.0f);
    HE_EXPECT_EQ(Vec4f_Zero.z, 0.0f);
    HE_EXPECT_EQ(Vec4f_Zero.w, 0.0f);
    HE_EXPECT_EQ(Vec4f_One.x, 1.0f);
    HE_EXPECT_EQ(Vec4f_One.y, 1.0f);
    HE_EXPECT_EQ(Vec4f_One.z, 1.0f);
    HE_EXPECT_EQ(Vec4f_One.w, 1.0f);
    HE_EXPECT_EQ(Vec4f_X.x, 1.0f);
    HE_EXPECT_EQ(Vec4f_X.y, 0.0f);
    HE_EXPECT_EQ(Vec4f_X.z, 0.0f);
    HE_EXPECT_EQ(Vec4f_X.w, 0.0f);
    HE_EXPECT_EQ(Vec4f_Y.x, 0.0f);
    HE_EXPECT_EQ(Vec4f_Y.y, 1.0f);
    HE_EXPECT_EQ(Vec4f_Y.z, 0.0f);
    HE_EXPECT_EQ(Vec4f_Y.w, 0.0f);
    HE_EXPECT_EQ(Vec4f_Z.x, 0.0f);
    HE_EXPECT_EQ(Vec4f_Z.y, 0.0f);
    HE_EXPECT_EQ(Vec4f_Z.z, 1.0f);
    HE_EXPECT_EQ(Vec4f_Z.w, 0.0f);
    HE_EXPECT_EQ(Vec4f_W.x, 0.0f);
    HE_EXPECT_EQ(Vec4f_W.y, 0.0f);
    HE_EXPECT_EQ(Vec4f_W.z, 0.0f);
    HE_EXPECT_EQ(Vec4f_W.w, 1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Vec2i)
{
    HE_EXPECT_EQ(Vec2i_Zero.x, 0);
    HE_EXPECT_EQ(Vec2i_Zero.y, 0);
    HE_EXPECT_EQ(Vec2i_One.x, 1);
    HE_EXPECT_EQ(Vec2i_One.y, 1);
    HE_EXPECT_EQ(Vec2i_X.x, 1);
    HE_EXPECT_EQ(Vec2i_X.y, 0);
    HE_EXPECT_EQ(Vec2i_Y.x, 0);
    HE_EXPECT_EQ(Vec2i_Y.y, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Vec3i)
{
    HE_EXPECT_EQ(Vec3i_Zero.x, 0);
    HE_EXPECT_EQ(Vec3i_Zero.y, 0);
    HE_EXPECT_EQ(Vec3i_Zero.z, 0);
    HE_EXPECT_EQ(Vec3i_One.x, 1);
    HE_EXPECT_EQ(Vec3i_One.y, 1);
    HE_EXPECT_EQ(Vec3i_One.z, 1);
    HE_EXPECT_EQ(Vec3i_X.x, 1);
    HE_EXPECT_EQ(Vec3i_X.y, 0);
    HE_EXPECT_EQ(Vec3i_X.z, 0);
    HE_EXPECT_EQ(Vec3i_Y.x, 0);
    HE_EXPECT_EQ(Vec3i_Y.y, 1);
    HE_EXPECT_EQ(Vec3i_Y.z, 0);
    HE_EXPECT_EQ(Vec3i_Z.x, 0);
    HE_EXPECT_EQ(Vec3i_Z.y, 0);
    HE_EXPECT_EQ(Vec3i_Z.z, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Vec4i)
{
    HE_EXPECT_EQ(Vec4i_Zero.x, 0);
    HE_EXPECT_EQ(Vec4i_Zero.y, 0);
    HE_EXPECT_EQ(Vec4i_Zero.z, 0);
    HE_EXPECT_EQ(Vec4i_Zero.w, 0);
    HE_EXPECT_EQ(Vec4i_One.x, 1);
    HE_EXPECT_EQ(Vec4i_One.y, 1);
    HE_EXPECT_EQ(Vec4i_One.z, 1);
    HE_EXPECT_EQ(Vec4i_One.w, 1);
    HE_EXPECT_EQ(Vec4i_X.x, 1);
    HE_EXPECT_EQ(Vec4i_X.y, 0);
    HE_EXPECT_EQ(Vec4i_X.z, 0);
    HE_EXPECT_EQ(Vec4i_X.w, 0);
    HE_EXPECT_EQ(Vec4i_Y.x, 0);
    HE_EXPECT_EQ(Vec4i_Y.y, 1);
    HE_EXPECT_EQ(Vec4i_Y.z, 0);
    HE_EXPECT_EQ(Vec4i_Y.w, 0);
    HE_EXPECT_EQ(Vec4i_Z.x, 0);
    HE_EXPECT_EQ(Vec4i_Z.y, 0);
    HE_EXPECT_EQ(Vec4i_Z.z, 1);
    HE_EXPECT_EQ(Vec4i_Z.w, 0);
    HE_EXPECT_EQ(Vec4i_W.x, 0);
    HE_EXPECT_EQ(Vec4i_W.y, 0);
    HE_EXPECT_EQ(Vec4i_W.z, 0);
    HE_EXPECT_EQ(Vec4i_W.w, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Quat)
{
    HE_EXPECT_EQ(Quat_Identity.x, 0.0f);
    HE_EXPECT_EQ(Quat_Identity.y, 0.0f);
    HE_EXPECT_EQ(Quat_Identity.z, 0.0f);
    HE_EXPECT_EQ(Quat_Identity.w, 1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Quata)
{
    HE_EXPECT_EQ(GetX(Quata_Identity.v), 0.0f);
    HE_EXPECT_EQ(GetY(Quata_Identity.v), 0.0f);
    HE_EXPECT_EQ(GetZ(Quata_Identity.v), 0.0f);
    HE_EXPECT_EQ(GetW(Quata_Identity.v), 1.0f);
    HE_EXPECT_EQ(Rotate(Quata_Identity, Vec4a_One), Vec4a_One);
    HE_EXPECT_EQ(Rotate(Quata_Identity, Vec4a_Zero), Vec4a_Zero);
    HE_EXPECT_EQ(Mul(Mul(Quata_Identity, Vec4a{ 1, 2, 3, 0 }), Conjugate(Quata_Identity)), (Quata{ 1, 2, 3, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, constants, Mat44)
{
    HE_EXPECT_EQ(TransformVector(Mat44_Identity, Vec4a_One), Vec4a_One);
    HE_EXPECT_EQ(TransformVector(Mat44_Identity, Vec4a_Zero), Vec4a_Zero);
}
