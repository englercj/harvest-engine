// Copyright Chad Engler

#include "vec_ulp_diff.h"

#include "he/math/quata.h"

#include "he/core/test.h"
#include "he/math/constants.h"
#include "he/math/mat44.h"
#include "he/math/types_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, MakeQuata)
{
    // Axis-angle
    Quata aa = MakeQuata(Vec4a_X, Float_PiHalf);
    HE_EXPECT(IsNormalized(aa));

    Vec4a axis;
    float angle;
    ToAxisAngle(axis, angle, aa);

    // Rotation matrix
    Mat44 m = MakeRotateMat44(aa);
    Quata rm = MakeQuata(m);
    HE_EXPECT(IsNormalized(rm));

    // Euler angles
    Vec4a angles = ToEuler(aa);
    Quata ea = MakeQuata(angles);
    HE_EXPECT(IsNormalized(ea));

    // Checks
    HE_EXPECT_EQ_ULP(aa, rm, 1);
    HE_EXPECT_EQ_ULP(aa, ea, 1);
    HE_EXPECT_EQ_ULP(ea, rm, 1);

    HE_EXPECT_EQ_ULP3(axis, Vec4a_X, 1);
    HE_EXPECT_EQ(angle, Float_PiHalf);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, ToEuler)
{
    Vec4a angles{ Float_PiQuarter, Float_PiQuarter, Float_PiQuarter, 0 };
    Quata q = MakeQuata(angles);

    Vec4a angles2 = ToEuler(q);

    HE_EXPECT_EQ_ULP(angles, angles2, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, ToAxisAngle)
{
    Vec4a normAxis = Normalize3(Vec4a_One);
    Quata q = MakeQuata(normAxis, Float_PiHalf);

    Vec4a axis;
    float angle;
    ToAxisAngle(axis, angle, q);

    HE_EXPECT_EQ_ULP3(axis, normAxis, 1);
    HE_EXPECT_EQ_ULP(angle, Float_PiHalf, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, IsNan)
{
    HE_EXPECT(IsNan(Quata{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Quata{ Float_Nan, Float_Nan, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Quata{ Float_Nan, Float_Nan, 1, Float_Nan }));
    HE_EXPECT(IsNan(Quata{ Float_Nan, 1, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Quata{ 1, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(IsNan(Quata{ Float_Nan, 1, 1, 1 }));
    HE_EXPECT(IsNan(Quata{ 1, Float_Nan, 1, 1 }));
    HE_EXPECT(IsNan(Quata{ 1, 1, Float_Nan, 1 }));
    HE_EXPECT(IsNan(Quata{ 1, 1, 1, Float_Nan }));
    HE_EXPECT(!IsNan(Quata{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(!IsNan(Quata{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, IsInfinite)
{
    HE_EXPECT(IsInfinite(Quata{ Float_Infinity, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Quata{ Float_Infinity, Float_Infinity, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Quata{ Float_Infinity, Float_Infinity, 1, Float_Infinity }));
    HE_EXPECT(IsInfinite(Quata{ Float_Infinity, 1, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Quata{ 1, Float_Infinity, Float_Infinity, Float_Infinity }));
    HE_EXPECT(IsInfinite(Quata{ Float_Infinity, 1, 1, 1 }));
    HE_EXPECT(IsInfinite(Quata{ 1, Float_Infinity, 1, 1 }));
    HE_EXPECT(IsInfinite(Quata{ 1, 1, Float_Infinity, 1 }));
    HE_EXPECT(IsInfinite(Quata{ 1, 1, 1, Float_Infinity }));
    HE_EXPECT(!IsInfinite(Quata{ Float_Nan, Float_Nan, Float_Nan, Float_Nan }));
    HE_EXPECT(!IsInfinite(Quata{ 1, 2, 3, 4 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, IsFinite)
{
    HE_EXPECT(IsFinite(Quata{ 1, 2, 3, 4 }));
    HE_EXPECT(!IsFinite(Quata{ Float_Infinity, 2, 3, 4 }));
    HE_EXPECT(!IsFinite(Quata{ 1, Float_Infinity, 3, 4 }));
    HE_EXPECT(!IsFinite(Quata{ 1, 2, Float_Infinity, 4 }));
    HE_EXPECT(!IsFinite(Quata{ 1, 2, 3, Float_Infinity }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, Conjugate)
{
    HE_EXPECT_EQ(Conjugate(Quata{ 1, 2, 3, 4 }), (Quata{ -1, -2, -3, 4 }));
    HE_EXPECT_EQ(Conjugate(Quata{ 1, -1, 1, -1 }), (Quata{ -1, 1, -1, -1 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, Mul)
{
    Vec4f a{ 1, 2, 3, 4 };
    Vec4f b{ 3, 4, 5, 6 };
    Vec3f av{ a.x, a.y, a.z };
    Vec3f bv{ b.x, b.y, b.z };
    Vec3f axis0 = av * b.w + bv * a.w + Cross(av, bv);
    float angle0 = a.w * b.w - Dot(av, bv);

    HE_EXPECT_EQ(Mul(Quata{ a.x, a.y, a.z, a.w }, Quata{ b.x, b.y, b.z, b.w }), (Quata{ axis0.x, axis0.y, axis0.z, angle0 }));

    Vec3f axis1 = av * b.w + Cross(av, bv);
    float angle1 = -Dot(av, bv);

    HE_EXPECT_EQ(Mul(MakeVec4a(a), Quata{ b.x, b.y, b.z, b.w }), (Quata{ axis1.x, axis1.y, axis1.z, angle1 }));

    Vec3f axis2 = bv * a.w + Cross(av, bv);
    float angle2 = -Dot(av, bv);

    HE_EXPECT_EQ(Mul(Quata{ a.x, a.y, a.z, a.w }, MakeVec4a(b)), (Quata{ axis2.x, axis2.y, axis2.z, angle2 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, Rotate_InvRotate)
{
    Quata qx = MakeQuata(Vec4a_X, 0.5f * Float_Pi);
    Quata qy = MakeQuata(Vec4a_Y, 0.5f * Float_Pi);
    Quata qz = MakeQuata(Vec4a_Z, 0.5f * Float_Pi);

    Vec4a v1 = Rotate(qz, Vec4a_X);
    Vec4a v2 = Rotate(qx, Vec4a_Y);
    Vec4a v3 = Rotate(qy, Vec4a_Z);

    HE_EXPECT_EQ_ULP(Dot3(v1, Vec4a_Y), 1, 1);
    HE_EXPECT_EQ_ULP(Dot3(v2, Vec4a_Z), 1, 1);
    HE_EXPECT_EQ_ULP(Dot3(v3, Vec4a_X), 1, 1);

    Vec4a v4 = InvRotate(qz, v1);
    Vec4a v5 = InvRotate(qx, v2);
    Vec4a v6 = InvRotate(qy, v3);

    HE_EXPECT_EQ_ULP(v4, Vec4a_X, 2);
    HE_EXPECT_EQ_ULP(v5, Vec4a_Y, 2);
    HE_EXPECT_EQ_ULP(v6, Vec4a_Z, 2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, Dot)
{
    HE_EXPECT_EQ(Dot(Quata{ 1, 2, 3, 4 }, Quata{ 0, 1, 2, 3 }), 20);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, Len)
{
    HE_EXPECT_EQ(Len(Quata{ 1, 2, 3, 4 }), Sqrt(30.f));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, Normalize)
{
    HE_EXPECT_EQ_ULP(Normalize(Quata{ 1, 1, 1, 1 }), (Quata{ 0.5f, 0.5f, 0.5f, 0.5f }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, IsNormalized)
{
    float x = Rcp(Sqrt(3.f));
    float y = Rcp(Sqrt(2.f));

    HE_EXPECT(IsNormalized(Quata{ 1, 0, 0, 0 }));
    HE_EXPECT(IsNormalized(Quata{ y, y, 0, 0 }));
    HE_EXPECT(IsNormalized(Quata{ x, x, x, 0 }));
    HE_EXPECT(IsNormalized(Quata{ 0.5f, 0.5f, 0.5f, 0.5f }));
    HE_EXPECT(!IsNormalized(Quata{ 1, 1, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, quata, Operators)
{
    Quata v;

    HE_EXPECT_EQ((-Quata{ 1, 2, 3, 4 }), (Quata{ -1, -2, -3, -4 }));

    v = Quata{ 1, 2, 3, 4 };
    v += Quata{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(v, (Quata{ 2, 4, 6, 8 }));
    HE_EXPECT_EQ((Quata{ 1, 2, 3, 4 } + Quata{ 1, 2, 3, 4 }), (Quata{ 2, 4, 6, 8 }));

    v = Quata{ 1, 2, 3, 4 };
    v -= Quata{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(v, (Quata{ 0, 0, 0, 0 }));
    HE_EXPECT_EQ((Quata{ 1, 2, 3, 4 } - Quata{ 1, 2, 3, 4 }), (Quata{ 0, 0, 0, 0 }));

    v = Quata{ 1, 2, 3, 4 };
    v *= Quata{ 1, 2, 3, 4 };
    HE_EXPECT_EQ(v, (Quata{ 1, 4, 9, 16 }));
    HE_EXPECT_EQ((Quata{ 1, 2, 3, 4 } * Quata{ 1, 2, 3, 4 }), (Quata{ 1, 4, 9, 16 }));

    Quata a = Quata{ 1, 2, 2, 1 };
    Quata b = Quata{ 2, 3, 3, 2 };

    HE_EXPECT_LT(a, b);
    HE_EXPECT_NE(a, b);
}
