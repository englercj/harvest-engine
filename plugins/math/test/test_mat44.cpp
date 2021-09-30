// Copyright Chad Engler

#include "vec_ulp_diff.h"

#include "he/math/mat44.h"

#include "he/core/test.h"
#include "he/math/constants.h"
#include "he/math/types_fmt.h"
#include "he/math/types_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, MakeScaleMat44)
{
    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeScaleMat44(1.0f);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeScaleMat44(Vec3f_One);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeScaleMat44(Vec4a_One);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 2, 0, 0, 0 },
            { 0, 2, 0, 0 },
            { 0, 0, 2, 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeScaleMat44(2.0f);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 2, 0, 0, 0 },
            { 0, 4, 0, 0 },
            { 0, 0, 6, 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeScaleMat44(Vec3f{ 2.0f, 4.0f, 6.0f });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 2, 0, 0, 0 },
            { 0, 4, 0, 0 },
            { 0, 0, 6, 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeScaleMat44(Vec4a{ 2.0f, 4.0f, 6.0f, 0.0f });
        HE_EXPECT_EQ(actual, expect);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, MakeRotateMat44)
{
    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeRotateMat44(Quat_Identity);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeRotateMat44(Quata_Identity);
        HE_EXPECT_EQ(actual, expect);
    }

    float halfAngle = 0.5f * Float_Pi;
    float s = sinf(halfAngle);
    float c = cosf(halfAngle);

    {
        Mat44 expect
        {
            { 1, 0, 0, 0 },
            { 0, 1 - (2 * s * s), 2 * c, 0 },
            { 0, -2 * c, 1 - (2 * s * s), 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeRotateMat44(Quat{ s, 0, 0, c });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1 - (2 * s * s), 0, -2 * c, 0 },
            { 0, 1, 0, 0 },
            { 2 * c, 0, 1 - (2 * s * s), 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeRotateMat44(Quat{ 0, s, 0, c });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1 - (2 * s * s), 2 * c, 0, 0 },
            { -2 * c, 1 - (2 * s * s), 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeRotateMat44(Quat{ 0, 0, s, c });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1, 0, 0, 0 },
            { 0, 1 - (2 * s * s), 2 * c, 0 },
            { 0, -2 * c, 1 - (2 * s * s), 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeRotateMat44(Quata{ s, 0, 0, c });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1 - (2 * s * s), 0, -2 * c, 0 },
            { 0, 1, 0, 0 },
            { 2 * c, 0, 1 - (2 * s * s), 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeRotateMat44(Quata{ 0, s, 0, c });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1 - (2 * s * s), 2 * c, 0, 0 },
            { -2 * c, 1 - (2 * s * s), 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = MakeRotateMat44(Quata{ 0, 0, s, c });
        HE_EXPECT_EQ(actual, expect);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, MakeTranslateMat44)
{
    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeTranslateMat44(Vec3f_Zero);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeRotateMat44(Vec4a_Zero);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 2, 4, 6, 1 },
        };
        Mat44 actual = MakeTranslateMat44(Vec3f{ 2, 4, 6 });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 2, 4, 6, 1 },
        };
        Mat44 actual = MakeTranslateMat44(Vec4a{ 2, 4, 6, 0 });
        HE_EXPECT_EQ(actual, expect);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, MakeTransformMat44)
{
    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeTransformMat44(Vec3f_Zero, Quat_Identity);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeTransformMat44(Vec4a_Zero, Quata_Identity);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeTransformMat44(Vec3f_Zero, Quat_Identity, Vec3f_One);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = MakeTransformMat44(Vec4a_Zero, Quata_Identity, Vec4a_One);
        HE_EXPECT_EQ(actual, expect);
    }

    float halfAngle = 0.5f * Float_Pi;
    float s = sinf(halfAngle);
    float c = cosf(halfAngle);

    {
        Mat44 expect
        {
            { 1, 0, 0, 0 },
            { 0, 1 - (2 * s * s), 2 * c, 0 },
            { 0, -2 * c, 1 - (2 * s * s), 0 },
            { 2, 4, 6, 1 },
        };
        Mat44 actual = MakeTransformMat44(Vec3f{ 2, 4, 6 }, Quat{ s, 0, 0, c });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1, 0, 0, 0 },
            { 0, 1 - (2 * s * s), 2 * c, 0 },
            { 0, -2 * c, 1 - (2 * s * s), 0 },
            { 2, 4, 6, 1 },
        };
        Mat44 actual = MakeTransformMat44(Vec4a{ 2, 4, 6, 1 }, Quata{ s, 0, 0, c });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 2, 0, 0, 0 },
            { 0, 3 * (1 - (2 * s * s)), 6 * c, 0 },
            { 0, -8 * c, 4 * (1 - (2 * s * s)), 0 },
            { 2, 4, 6, 1 },
        };
        Mat44 actual = MakeTransformMat44(Vec3f{ 2, 4, 6 }, Quat{ s, 0, 0, c }, Vec3f{ 2, 3, 4 });
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 2, 0, 0, 0 },
            { 0, 3 * (1 - (2 * s * s)), 6 * c, 0 },
            { 0, -8 * c, 4 * (1 - (2 * s * s)), 0 },
            { 2, 4, 6, 1 },
        };
        Mat44 actual = MakeTransformMat44(Vec4a{ 2, 4, 6, 1 }, Quata{ s, 0, 0, c }, Vec4a{ 2, 3, 4, 1 });
        HE_EXPECT_EQ(actual, expect);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, Mul)
{
    {
        Mat44 expect
        {
            { 2, 0, 0, 0 },
            { 0, 3, 0, 0 },
            { 0, 0, 4, 0 },
            { 2, 4, 6, 1 },
        };
        Mat44 actual = Mul(Mat44_Identity, expect);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 m0 = MakeTranslateMat44(Vec3f{ 1, 0, 0 });
        Mat44 m1 = MakeScaleMat44(0.5f);
        Mat44 expect
        {
            { 0.5f, 0, 0, 0 },
            { 0, 0.5f, 0, 0 },
            { 0, 0, 0.5f, 0 },
            { 1, 0, 0, 1 },
        };
        Mat44 actual = Mul(m0, m1);
        HE_EXPECT_EQ(actual, expect);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, Inverse)
{
    Mat44 m0 = Inverse(MakeTranslateMat44(Vec3f{ 1, 0, 0 }));
    Mat44 m1 = Inverse(MakeScaleMat44(0.5f));

    HE_EXPECT_EQ_ULP(TransformPoint(m0, Vec3f{ 0, 0, 0 }), (Vec3f{ -1, 0, 0 }), 1);
    HE_EXPECT_EQ_ULP(TransformPoint(m1, Vec3f{ 1, 1, 1 }), (Vec3f{ 2, 2, 2 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, InverseTransform)
{
    Mat44 m0 = InverseTransform(MakeTranslateMat44(Vec3f{ 1, 0, 0 }));
    Mat44 m1 = InverseTransform(MakeScaleMat44(0.5f));

    HE_EXPECT_EQ_ULP(TransformPoint(m0, Vec3f{ 0, 0, 0 }), (Vec3f{ -1, 0, 0 }), 1);
    HE_EXPECT_EQ_ULP(TransformPoint(m1, Vec3f{ 1, 1, 1 }), (Vec3f{ 2, 2, 2 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, InverseTransformNoScale)
{
    Mat44 m0 = InverseTransformNoScale(MakeTranslateMat44(Vec3f{ 1, 0, 0 }));

    HE_EXPECT_EQ(TransformPoint(m0, Vec3f{ 0, 0, 0 }), (Vec3f{ -1, 0, 0 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, Transpose)
{
    {
        Mat44 expect = Mat44_Identity;
        Mat44 actual = Transpose(Mat44_Identity);
        HE_EXPECT_EQ(actual, expect);
    }

    {
        Mat44 expect
        {
            { 1, 0, 0, 2 },
            { 0, 1, 0, 4 },
            { 0, 0, 1, 6 },
            { 0, 0, 0, 1 },
        };
        Mat44 actual = Transpose(MakeTranslateMat44(Vec3f{ 2, 4, 6 }));
        HE_EXPECT_EQ(actual, expect);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, ViewLH)
{
    Mat44 lh0 = ViewLH(Vec4a_Zero, Vec4a_Z, Vec4a_Y);

    Vec4a expect{ 1, 2, 3, 1 };
    Vec4a actual = TransformVector(lh0, Vec4a{ 1, 2, 3, 1 });
    HE_EXPECT_EQ_ULP(actual, expect, 2);

    Mat44 lh1 = ViewLH(Vec3f{ 0, 0, 0 }, Vec3f{ 0, 0, 1 }, Vec3f{ 0, 1, 0 });
    HE_EXPECT_EQ(lh0, lh1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, ViewRH)
{
    Mat44 rh0 = ViewRH(Vec4a_Zero, Vec4a_Z, Vec4a_Y);

    Vec4a expect{ -1, 2, 3, 1 };
    Vec4a actual = TransformVector(rh0, Vec4a{ 1, 2, 3, 1 });
    HE_EXPECT_EQ_ULP(actual, expect, 2);

    Mat44 rh1 = ViewRH(Vec3f{ 0, 0, 0 }, Vec3f{ 0, 0, 1 }, Vec3f{ 0, 1, 0 });
    HE_EXPECT_EQ(rh0, rh1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, OrthoLH)
{
    Vec4a a{ -5.0f, -5.0f, -5.0f, 1 };
    Vec4a b{ 5.0f,  5.0f,  5.0f, 1 };

    Mat44 m = OrthoLH(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);

    Vec4a a1 = ProjectPoint(m, a);
    Vec4a b1 = ProjectPoint(m, b);

    HE_EXPECT_EQ_ULP(a1, (Vec4a{ -1, -1, 0, 1 }), 1);
    HE_EXPECT_EQ_ULP(b1, (Vec4a{ 1, 1, 1, 1 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, OrthoRH)
{
    Vec4a a{ -5.0f, -5.0f, -5.0f, 1 };
    Vec4a b{ 5.0f,  5.0f,  5.0f, 1 };

    Mat44 m = OrthoRH(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);

    Vec4a a2 = ProjectPoint(m, a);
    Vec4a b2 = ProjectPoint(m, b);

    HE_EXPECT_EQ_ULP(a2, (Vec4a{ -1, -1, 1, 1 }), 1);
    HE_EXPECT_EQ_ULP(b2, (Vec4a{ 1, 1, 0, 1 }), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, PerspLH)
{
    Vec4a n{ 0, 0, 1.0f, 0 };
    Vec4a f{ 0, 0, 1000.0f, 0 };

    Mat44 m = PerspLH(ToRadians(90.0f), 1.0f, 1.0f, 1000.0f);

    Vec4a n1 = ProjectPoint(m, n);
    Vec4a f1 = ProjectPoint(m, f);
    HE_EXPECT_EQ(GetZ(n1), 0.0f);
    HE_EXPECT_EQ(GetZ(f1), 1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, PerspRH)
{
    Vec4a n{ 0, 0, 1.0f, 0 };
    Vec4a f{ 0, 0, 1000.0f, 0 };

    Mat44 m = PerspRH(ToRadians(90.0f), 1.0f, 1.0f, 1000.0f);

    Vec4a n2 = ProjectPoint(m, Negate(n));
    Vec4a f2 = ProjectPoint(m, Negate(f));
    HE_EXPECT_EQ(GetZ(n2), 0.0f);
    HE_EXPECT_EQ(GetZ(f2), 1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, IRZPerspLH)
{
    Vec4a n{ 0, 0, 1.0f, 0 };
    Vec4a f{ 0, 0, Float_Max, 0 };

    Mat44 m = IRZPerspLH(ToRadians(90.0f), 1.0f, 1.0f);

    Vec4a n1 = ProjectPoint(m, n);
    Vec4a f1 = ProjectPoint(m, f);
    HE_EXPECT_EQ_ULP(GetZ(n1), 1.0f, 1);
    HE_EXPECT_LT(Abs(GetZ(f1)), Float_Min);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, IRZPerspRH)
{
    Vec4a n{ 0, 0, 1.0f, 0 };
    Vec4a f{ 0, 0, Float_Max, 0 };

    Mat44 m = IRZPerspRH(ToRadians(90.0f), 1.0f, 1.0f);

    Vec4a n2 = ProjectPoint(m, Negate(n));
    Vec4a f2 = ProjectPoint(m, Negate(f));
    HE_EXPECT_EQ_ULP(GetZ(n2), 1.0f, 1);
    HE_EXPECT_LT(Abs(GetZ(f2)), Float_Min);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, TransformVector)
{
    Vec4a v = (Vec4a{ 1, 2, 3, 1 });

    Mat44 m = MakeTransformMat44(v, Quata_Identity, Vec4a_One);
    Vec4a v2 = TransformVector(m, v);
    HE_EXPECT_EQ(v2, SetW(v + v, 1.0f));

    Vec4f v3 = TransformVector(m, MakeVec4<float>(v));
    HE_EXPECT_EQ(v3, MakeVec4<float>(SetW(v + v, 1.0f)));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, TransformVector3)
{
    Vec4a v = (Vec4a{ 1, 2, 3, 1 });

    Mat44 m = MakeTransformMat44(v, Quata_Identity, Vec4a_One);
    Vec4a v2 = TransformVector3(m, v);
    HE_EXPECT(All3(Eq(v2, v)));

    Vec3f v3 = TransformVector(m, MakeVec3<float>(v));
    HE_EXPECT_EQ(v3, MakeVec3<float>(v));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, TransformPoint)
{
    Vec4a s = (Vec4a{ 1, 1, 1, 0 });
    Vec4a v = (Vec4a{ 1, 2, 3, 0 });

    Mat44 m = MakeTransformMat44(v, Quata_Identity, s);
    Vec4a v2 = TransformPoint(m, v);
    HE_EXPECT(All3(Eq(v2, (Vec4a{ 2, 4, 6, 1 }))));

    Vec3f v3 = TransformPoint(m, MakeVec3<float>(v));
    HE_EXPECT_EQ(v3, (Vec3f{ 2, 4, 6 }));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, ProjectPoint)
{
    Mat44 m = PerspLH(0.5f, 1.0f, 1.0f, 1000.0f);

    HE_EXPECT_EQ(GetZ(ProjectPoint(m, (Vec4a{ 0, 0, 1, 1 }))), 0.0f);
    HE_EXPECT_EQ(GetZ(ProjectPoint(m, (Vec4a{ 0, 0, 1000, 1 }))), 1.0f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, operator_eq)
{
    HE_EXPECT_EQ(Mat44_Identity, Mat44_Identity);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(math, mat44, operator_ne)
{
    HE_EXPECT_NE(Mat44_Identity, Mul(Mat44_Identity, MakeScaleMat44(2)));
}
