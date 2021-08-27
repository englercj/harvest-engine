// Copyright Chad Engler

#pragma once

#include "he/math/constants.h"
#include "he/math/types.h"
#include "he/math/vec3.h"
#include "he/math/vec4.h"
#include "he/math/vec4a.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    // Returns a scale matrix from uniform scale `s`.
    Mat44 MakeScaleMat44(float s);

    // Returns a scale matrix from scale `s`.
    Mat44 MakeScaleMat44(const Vec3f& s);

    // Returns a scale matrix from scale `s`.
    Mat44 MakeScaleMat44(const Vec4a& s);

    // Returns a rotation matrix from rotation `q`.
    Mat44 MakeRotateMat44(const Quat& q);

    // Returns a rotation matrix from rotation `q`.
    Mat44 MakeRotateMat44(const Quata& q);

    // Returns a translation matrix from position `p`.
    Mat44 MakeTranslateMat44(const Vec3f& p);

    // Returns a translation matrix from position `p`.
    Mat44 MakeTranslateMat44(const Vec4a& p);

    // Returns a transformation matrix from position `p`, rotation `q`, and identity scale.
    Mat44 MakeTransformMat44(const Vec3f& p, const Quat& q);

    // Returns a transformation matrix from position `p`, rotation `q`, and identity scale.
    Mat44 MakeTransformMat44(const Vec4a& p, const Quata& q);

    // Returns a transformation matrix from position `p`, rotation `q`, and scale `s`.
    Mat44 MakeTransformMat44(const Vec3f& p, const Quat& q, const Vec3f& s);

    // Returns a transformation matrix from position `p`, rotation `q`, and scale `s`.
    Mat44 MakeTransformMat44(const Vec4a& p, const Quata& q, const Vec4a& s);

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    // Returns the multiplication of `a` and `b`. This has the effect of `b` transforming `a`.
    Mat44 Mul(const Mat44& a, const Mat44& b);

    // Returns the inverse of matrix `m`.
    Mat44 Inverse(const Mat44& m);

    // Returns the inverse of matrix `m`, assuming that `m` is a tranform matrix.
    Mat44 InverseTransform(const Mat44& m);

    // Returns the inverse of matrix `m`, assuming that `m` is a tranform matrix and the scale is `1`.
    Mat44 InverseTransformNoScale(const Mat44& m);

    // --------------------------------------------------------------------------------------------
    // Geometric

    // Returns the transpose matrix of `m`.
    Mat44 Transpose(const Mat44& m);

    // Returns a left-handed view matrix at `pos` oriented to the `forward` and `up` axes.
    Mat44 ViewLH(const Vec3f& pos, const Vec3f& forward, const Vec3f& up);

    // Returns a left-handed view matrix at `pos` oriented to the `forward` and `up` axes.
    Mat44 ViewLH(const Vec4a& pos, const Vec4a& forward, const Vec4a& up);

    // Returns a right-handed view matrix at `pos` oriented to the `forward` and `up` axes.
    Mat44 ViewRH(const Vec3f& pos, const Vec3f& forward, const Vec3f& up);

    // Returns a right-handed view matrix at `pos` oriented to the `forward` and `up` axes.
    Mat44 ViewRH(const Vec4a& pos, const Vec4a& forward, const Vec4a& up);

    // Returns left-handed an orthographic projection matrix with
    //  `x` and `y` mapped to `[-1,1]` based on dimensions `[minX,maxX]` and `[minY,maxY]`, and
    //  `z` mapped to `[0,1]` based on clipping planes `zNear` and `zFar`.
    Mat44 OrthoLH(float minX, float maxX, float minY, float maxY, float zNear, float zFar);

    // Returns right-handed an orthographic projection matrix with
    //  `x` and `y` mapped to `[-1,1]` based on dimensions `[minX,maxX]` and `[minY,maxY]`, and
    //  `z` mapped to `[0,1]` based on clipping planes `zNear` and `zFar`.
    Mat44 OrthoRH(float minX, float maxX, float minY, float maxY, float zNear, float zFar);

    // Returns a left-handed perspective projection matrix base on an FOV with
    // `x` and `y` mapped to `[-1,1]` based on `aspectRatio`,
    // `z` mapped to `[0,1]` based on a vertical field of view `fov` and clipping planes `zNear` and `zFar`.
    Mat44 PerspLH(float fov, float aspectRatio, float zNear, float zFar);

    // Returns a right-handed perspective projection matrix base on an FOV with
    // `x` and `y` mapped to `[-1,1]` based on `aspectRatio`,
    // `z` mapped to `[0,1]` based on a vertical field of view `fov` and clipping planes `zNear` and `zFar`.
    Mat44 PerspRH(float fov, float aspectRatio, float zNear, float zFar);

    // Returns a left-handed infinite-reversed perspective projection matrix with
    // `x` and `y` mapped to `[-1,1]` based on `aspectRatio`, and
    // `z` mapped to `(1,0]` based on a vertical field of view `fov` and `zNear` clipping plane.
    Mat44 IRZPerspLH(float fov, float aspectRatio, float zNear);

    // Returns a right-handed infinite-reversed perspective projection matrix with
    // `x` and `y` mapped to `[-1,1]` based on `aspectRatio`, and
    // `z` mapped to `(1,0]` based on a vertical field of view `fov` and `zNear` clipping plane.
    Mat44 IRZPerspRH(float fov, float aspectRatio, float zNear);

    // Returns the multiplication of `v` and `m`.
    Vec4f TransformVector(const Mat44& m, const Vec4f& v);

    // Returns the multiplication of `v` and `m`.
    Vec4a TransformVector(const Mat44& m, const Vec4a& v);

    // Returns the multiplication of `v` and `m` assuming `v.w == 0`.
    Vec3f TransformVector(const Mat44& m, const Vec3f& v);

    // Returns the multiplication of `v` and `m` assuming `v.w == 0`.
    Vec4a TransformVector3(const Mat44& m, const Vec4a& v);

    // Returns the multiplication of `v` and `m` assuming `v.w == 1` with no perspective divide.
    Vec3f TransformPoint(const Mat44& m, const Vec3f& v);

    // Returns the multiplication of `v` and `m` assuming `v.w == 1` with no perspective divide.
    Vec4a TransformPoint(const Mat44& m, const Vec4a& v);

    // Returns the multiplication of `v` and `m` assuming `v.w == 1` with perspective divide.
    Vec3f ProjectPoint(const Mat44& m, const Vec3f& v);

    // Returns the multiplication of `v` and `m` assuming `v.w == 1` with perspective divide.
    Vec4a ProjectPoint(const Mat44& m, const Vec4a& v);

    // --------------------------------------------------------------------------------------------
    // Operators

    bool operator==(const Mat44& a, const Mat44& b);
    bool operator!=(const Mat44& a, const Mat44& b);
}

#include "he/math/inline/mat44.inl"

#if HE_SIMD_SSE2
    #include "he/math/inline/mat44.sse.inl"
#elif HE_SIMD_NEON
    #include "he/math/inline/mat44.neon.inl"
#else
    #include "he/math/inline/mat44.seq.inl"
#endif
