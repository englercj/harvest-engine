// Copyright Chad Engler

#include "vec_ulp_diff.h"

#include "he/math/quat.h"

#include "he/core/test.h"
#include "he/math/constants.h"
#include "he/math/mat44.h"
#include "he/math/types_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, MakeQuat)
{
    // Axis-angle
    Quat aa = MakeQuat(Vec3f_X, Float_PiHalf);
    HE_EXPECT(IsNormalized(aa));

    Vec3f axis;
    float angle;
    ToAxisAngle(axis, angle, aa);
    HE_EXPECT_EQ_ULP(axis, Vec3f_X, 1);
    HE_EXPECT_EQ(angle, Float_PiHalf);

    // Rotation matrix
    Mat44 m = MakeRotateMat44(aa);
    Quat rm = MakeQuat(m);
    HE_EXPECT(IsNormalized(rm));

    // Euler angles
    Vec3f angles = ToEuler(aa);
    Quat ea = MakeQuat(angles);
    HE_EXPECT(IsNormalized(ea));

    // Checks
    HE_EXPECT_EQ_ULP(aa, rm, 2);
    HE_EXPECT_EQ_ULP(aa, ea, 2);
    HE_EXPECT_EQ_ULP(ea, rm, 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, ToEuler)
{
    Vec3f angles{ Float_PiQuarter, Float_PiQuarter, Float_PiQuarter };
    Quat q = MakeQuat(angles);

    Vec3f angles2 = ToEuler(q);

    HE_EXPECT_EQ_ULP(angles, angles2, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, ToAxisAngle)
{
    Vec3f normAxis = Normalize(Vec3f_One);
    Quat q = MakeQuat(normAxis, Float_PiHalf);

    Vec3f axis;
    float angle;
    ToAxisAngle(axis, angle, q);

    HE_EXPECT_EQ_ULP(axis, normAxis, 1);
    HE_EXPECT_EQ_ULP(angle, Float_PiHalf, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, IsNan)
{
    HE_EXPECT(IsNan(Quat{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Quat{ Float_Nan, Float_Nan, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Quat{ Float_Nan, Float_Nan, 1, Float_Nan }));
    HE_EXPECT(IsNan(Quat{ Float_Nan, 1, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Quat{ 1, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Quat{ Float_Nan, 1, 1, 1 }));
    HE_EXPECT(IsNan(Quat{ 1, Float_Nan, 1, 1 }));
    HE_EXPECT(IsNan(Quat{ 1, 1, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Quat{ 1, 1, 1, Float_Nan }));
    HE_EXPECT(!IsNan(Quat{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(!IsNan(Quat{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, IsInfinite)
{
    HE_EXPECT(IsInfinite(Quat{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Quat{ Float_Infinity, Float_Infinity, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Quat{ Float_Infinity, Float_Infinity, 1, Float_Infinity }));
    HE_EXPECT(IsInfinite(Quat{ Float_Infinity, 1, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Quat{ 1, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Quat{ Float_Infinity, 1, 1, 1 }));
    HE_EXPECT(IsInfinite(Quat{ 1, Float_Infinity, 1, 1 }));
    HE_EXPECT(IsInfinite(Quat{ 1, 1, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Quat{ 1, 1, 1, Float_Infinity }));
    HE_EXPECT(!IsInfinite(Quat{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(!IsInfinite(Quat{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, IsFinite)
{
    HE_EXPECT(IsFinite(Quat{ 1, 2, 3, 4 }));
    HE_EXPECT(!IsFinite(Quat{ Float_Infinity, 2, 3, 4 }));
    HE_EXPECT(!IsFinite(Quat{ 1, Float_Infinity, 3, 4 }));
    HE_EXPECT(!IsFinite(Quat{ 1, 2, Float_Infinity, 4 }));
    HE_EXPECT(!IsFinite(Quat{ 1, 2, 3, Float_Infinity }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, Conjugate)
{
    HE_EXPECT_EQ(Conjugate(Quat{ 1, 2, 3, 4 }), (Quat{ -1, -2, -3, 4 }));
    HE_EXPECT_EQ(Conjugate(Quat{ 1, -1, 1, -1 }), (Quat{ -1, 1, -1, -1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, Mul)
{
    Quat a{ 1, 2, 3, 4 };
    Quat b{ 3, 4, 5, 6 };
    Vec3f av{ a.x, a.y, a.z };
    Vec3f bv{ b.x, b.y, b.z };
    Vec3f axis0 = av * b.w + bv * a.w + Cross(av, bv);
    float angle0 = a.w * b.w - Dot(av, bv);

    HE_EXPECT_EQ(Mul(a, b), (Quat{ axis0.x, axis0.y, axis0.z, angle0 }));

    Vec3f axis1 = av * b.w + Cross(av, bv);
    float angle1 = -Dot(av, bv);

    HE_EXPECT_EQ(Mul(av, b), (Quat{ axis1.x, axis1.y, axis1.z, angle1 }));

    Vec3f axis2 = bv * a.w + Cross(av, bv);
    float angle2 = -Dot(av, bv);

    HE_EXPECT_EQ(Mul(a, bv), (Quat{ axis2.x, axis2.y, axis2.z, angle2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, Rotate_InvRotate)
{
    Quat qx = MakeQuat(Vec3f_X, 0.5f * Float_Pi);
    Quat qy = MakeQuat(Vec3f_Y, 0.5f * Float_Pi);
    Quat qz = MakeQuat(Vec3f_Z, 0.5f * Float_Pi);

    Vec3f v1 = Rotate(qz, Vec3f_X);
    Vec3f v2 = Rotate(qx, Vec3f_Y);
    Vec3f v3 = Rotate(qy, Vec3f_Z);

    HE_EXPECT_EQ_ULP(Dot(v1, Vec3f_Y), 1, 1);
    HE_EXPECT_EQ_ULP(Dot(v2, Vec3f_Z), 1, 1);
    HE_EXPECT_EQ_ULP(Dot(v3, Vec3f_X), 1, 1);

    Vec3f v4 = InvRotate(qz, v1);
    Vec3f v5 = InvRotate(qx, v2);
    Vec3f v6 = InvRotate(qy, v3);

    HE_EXPECT_EQ_ULP(v4, Vec3f_X, 2);
    HE_EXPECT_EQ_ULP(v5, Vec3f_Y, 2);
    HE_EXPECT_EQ_ULP(v6, Vec3f_Z, 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, Dot)
{
    HE_EXPECT_EQ(Dot(Quat{ 1, 2, 3, 4 }, Quat{ 0, 1, 2, 3 }), 20);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, Len)
{
    HE_EXPECT_EQ(Len(Quat{ 1, 2, 3, 4 }), Sqrt(30.f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, Normalize)
{
    HE_EXPECT_EQ_ULP(Normalize(Quat{ 1, 1, 1, 1 }), (Quat{ 0.5f, 0.5f, 0.5f, 0.5f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, IsNormalized)
{
    float x = Rcp(Sqrt(3.f));
    float y = Rcp(Sqrt(2.f));

    HE_EXPECT(IsNormalized(Quat{ 1, 0, 0, 0 }));
    HE_EXPECT(IsNormalized(Quat{ y, y, 0, 0 }));
    HE_EXPECT(IsNormalized(Quat{ x, x, x, 0 }));
    HE_EXPECT(IsNormalized(Quat{ 0.5f, 0.5f, 0.5f, 0.5f }));
    HE_EXPECT(!IsNormalized(Quat{ 1, 1, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quat, Operators)
{
    Quat v;

    HE_EXPECT_EQ((-Quat{ 1, 2, 3, 4 }), (Quat{ -1, -2, -3, -4 }));

    v = { 1, 2, 3, 4 };
    v += { 1, 2, 3, 4 };
    HE_EXPECT_EQ(v, (Quat{ 2, 4, 6, 8 }));
    HE_EXPECT_EQ((Quat{ 1, 2, 3, 4 } + Quat{ 1, 2, 3, 4 }), (Quat{ 2, 4, 6, 8 }));

    v = { 1, 2, 3, 4 };
    v -= { 1, 2, 3, 4 };
    HE_EXPECT_EQ(v, (Quat{ 0, 0, 0, 0 }));
    HE_EXPECT_EQ((Quat{ 1, 2, 3, 4 } - Quat{ 1, 2, 3, 4 }), (Quat{ 0, 0, 0, 0 }));

    v = { 1, 2, 3, 4 };
    v *= 2;
    HE_EXPECT_EQ(v, (Quat{ 2, 4, 6, 8 }));
    HE_EXPECT_EQ((Quat{ 1, 2, 3, 4 } * 2), (Quat{ 2, 4, 6, 8 }));

    Quat q1 = MakeQuat(Vec3f_Z, 0.5f * Float_Pi);
    HE_EXPECT_EQ_ULP(Dot(Rotate(q1, Vec3f{ 1, 0, 0 }), Vec3f{ 0, 1, 0 }), 1, 1);
    Quat q2 = MakeQuat(Vec3f_Z, 0.5f * Float_Pi);
    q1 *= q2;
    HE_EXPECT_EQ_ULP(Dot(Rotate(q1, Vec3f{ 1, 0, 0 }), Vec3f{ -1, 0, 0 }), 1, 4);

    q1 = MakeQuat(Vec3f_Z, 0.5f * Float_Pi);
    Quat q3 = q1 * q2;
    HE_EXPECT_EQ_ULP(Dot(Rotate(q3, Vec3f{ 1, 0, 0 }), Vec3f{ -1, 0, 0 }), 1, 4);

    HE_EXPECT_LT((Quat{ 1, 1, 1, 1 }), (Quat{ 2, 0, 0, 0 }));
    HE_EXPECT_LT((Quat{ 1, 1, 1, 1 }), (Quat{ 1, 2, 0, 0 }));
    HE_EXPECT_LT((Quat{ 1, 1, 1, 1 }), (Quat{ 1, 1, 2, 0 }));
    HE_EXPECT_LT((Quat{ 1, 1, 1, 1 }), (Quat{ 1, 1, 1, 2 }));

    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } < Quat{ 0, 2, 2, 2 }));
    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } < Quat{ 1, 0, 2, 2 }));
    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } < Quat{ 1, 1, 0, 2 }));
    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } < Quat{ 1, 1, 1, 0 }));

    HE_EXPECT((Quat{ 1, 1, 1, 1 } == Quat{ 1, 1, 1, 1 }));
    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } == Quat{ 0, 1, 1, 1 }));
    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } == Quat{ 1, 0, 1, 1 }));
    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } == Quat{ 1, 1, 0, 1 }));
    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } == Quat{ 1, 1, 1, 0 }));

    HE_EXPECT(!(Quat{ 1, 1, 1, 1 } != Quat{ 1, 1, 1, 1 }));
    HE_EXPECT((Quat{ 1, 1, 1, 1 } != Quat{ 0, 1, 1, 1 }));
    HE_EXPECT((Quat{ 1, 1, 1, 1 } != Quat{ 1, 0, 1, 1 }));
    HE_EXPECT((Quat{ 1, 1, 1, 1 } != Quat{ 1, 1, 0, 1 }));
    HE_EXPECT((Quat{ 1, 1, 1, 1 } != Quat{ 1, 1, 1, 0 }));
}
