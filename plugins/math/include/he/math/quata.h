// Copyright Chad Engler

#pragma once

#include "he/core/cpu.h"
#include "he/math/quat.h"
#include "he/math/types.h"
#include "he/math/vec4a.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    // Returns a Quaternion from a rotation matrix `m`.
    Quata MakeQuata(const Mat44& m);

    // Returns a Quaternion from the Euler angles `angles` in radians.
    // The angles are expected to take the form Roll (x), Pitch (y), Yaw (z).
    Quata MakeQuata(const Vec4a& angles);

    // Returns a Quaternion from a rotation of `angle` radians around `axis`.
    Quata MakeQuata(const Vec4a& axis, float angle);

    // Returns a representation of `q` as Euler angles (pitch, yaw, roll) in radians.
    // The angles are take the form Roll (x), Pitch (y), Yaw (z).
    Vec4a ToEuler(const Quata& q);

    // Returns the axis-angle representation of `q` in `outAxis` and `outAngle`.
    void ToAxisAngle(Vec4a& outAxis, float& outAngle, const Quata& q);

    // --------------------------------------------------------------------------------------------
    // Classification

    // Returns true if any component of `q` is NaN.
    bool IsNan(const Quata& q);

    // Returns true if any component of `q` is infinite and is not NaN.
    bool IsInfinite(const Quata& q);

    // Returns true if all the components of `q` are not infite and not NaN.
    bool IsFinite(const Quata& q);

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    // Returns the conjugate of Quaternion `q`.
    Quata Conjugate(const Quata& q);

    // Returns the multiplication of Quaternions `a` and `b`.
    // Equivalent to the notation `b * a` where `a` is transformed by `b`.
    Quata Mul(const Quata& a, const Quata& b);

    // Returns the multiplication of vector `v` and Quaternion `q`, treating the vector as a Quaternion with w == 0.
    Quata Mul(const Vec4a& v, const Quata& q);

    // Returns the multiplication of Quaternion `q` and vector `v`, treating the vector as a Quaternion with w == 0.
    Quata Mul(const Quata& q, const Vec4a& v);

    // --------------------------------------------------------------------------------------------
    // Geometric

    // Returns the rotation of vector `v` by Quaternion `q`.
    Vec4a Rotate(const Quata& q, const Vec4a& v);

    // Returns the inverse rotation of vector `v` by Quaternion `q`.
    Vec4a InvRotate(const Quata& q, const Vec4a& v);

    // Returns the dot product of `a` and `b`.
    float Dot(const Quata& a, const Quata& b);

    // Returns the length of `q`.
    float Len(const Quata& q);

    // Returns a normalized Quaternion from `q`.
    Quata Normalize(const Quata& q);

    // Returns true if `q` is a normalized vector.
    bool IsNormalized(const Quata& q);

    // --------------------------------------------------------------------------------------------
    // Operators

    Quata operator-(const Quata& q);

    Quata operator+(const Quata& a, const Quata& b);
    Quata operator-(const Quata& a, const Quata& b);
    Quata operator*(const Quata& a, const Quata& b);

    Quata operator*(const Quata& a, float b);

    Quata& operator+=(Quata& a, const Quata& b);
    Quata& operator-=(Quata& a, const Quata& b);
    Quata& operator*=(Quata& a, const Quata& b);

    Quata& operator*=(Quata& a, float b);

    bool operator<(const Quata& a, const Quata& b);
    bool operator==(const Quata& a, const Quata& b);
    bool operator!=(const Quata& a, const Quata& b);
}

#include "he/math/inline/quata.inl"

#if HE_SIMD_SSE2
    #include "he/math/inline/quata.sse.inl"
#elif HE_SIMD_NEON
    #include "he/math/inline/quata.neon.inl"
#else
    #include "he/math/inline/quata.seq.inl"
#endif
