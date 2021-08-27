// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/math/float.h"
#include "he/math/types.h"
#include "he/math/vec3.h"
#include "he/math/vec4.h"
#include "he/math/vec4a.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Conversion

    // Returns a Quaternion from a rotation matrix `m`.
    Quat MakeQuat(const Mat44& m);

    // Returns a Quaternion from the Euler angles `angles` in radians.
    // The angles are expected to take the form Roll (x), Pitch (y), Yaw (z).
    Quat MakeQuat(const Vec3f& angles);

    // Returns a Quaternion from a rotation of `angle` radians around `axis`.
    Quat MakeQuat(const Vec3f& axis, float angle);

    // Returns a representation of `q` as Euler angles (pitch, yaw, roll) in radians.
    // The angles are take the form Roll (x), Pitch (y), Yaw (z).
    Vec3f ToEuler(const Quat& q);

    // Returns the axis-angle representation of `q` in `outAxis` and `outAngle`.
    void ToAxisAngle(Vec3f& outAxis, float& outAngle, const Quat& q);

    // --------------------------------------------------------------------------------------------
    // Classification

    // Returns true if any component of `q` is NaN.
    constexpr bool IsNan(const Quat& q);

    // Returns true if any component of `q` is infinite and is not NaN.
    constexpr bool IsInfinite(const Quat& q);

    // Returns true if all the components of `q` are not infite and not NaN.
    constexpr bool IsFinite(const Quat& q);

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    // Returns the conjugate of Quaternion `q`.
    constexpr Quat Conjugate(const Quat& q);

    // Returns the multiplication of Quaternions `a` and `b`.
    // Equivalent to the notation `b * a` where `a` is transformed by `b`.
    constexpr Quat Mul(const Quat& a, const Quat& b);

    // Returns the multiplication of vector `v` and Quaternion `q`, treating the vector as a Quaternion with w == 0.
    constexpr Quat Mul(const Vec3f& v, const Quat& q);

    // Returns the multiplication of Quaternion `q` and vector `v`, treating the vector as a Quaternion with w == 0.
    constexpr Quat Mul(const Quat& q, const Vec3f& v);

    // --------------------------------------------------------------------------------------------
    // Geometric

    // Returns the rotation of vector `v` by Quaternion `q`.
    Vec3f Rotate(const Quat& q, const Vec3f& v);

    // Returns the inverse rotation of vector `v` by Quaternion `q`.
    Vec3f InvRotate(const Quat& q, const Vec3f& v);

    // Returns the dot product of `a` and `b`.
    constexpr float Dot(const Quat& a, const Quat& b);

    // Returns the length of `q`.
    float Len(const Quat& q);

    // Returns a normalized Quaternion from `q`.
    Quat Normalize(const Quat& q);

    // Returns true if `q` is a normalized vector.
    bool IsNormalized(const Quat& q);

    // --------------------------------------------------------------------------------------------
    // Operators

    constexpr Quat operator-(const Quat& v);

    constexpr Quat operator+(const Quat& a, const Quat& b);
    constexpr Quat operator-(const Quat& a, const Quat& b);
    constexpr Quat operator*(const Quat& a, const Quat& b);

    constexpr Quat operator*(const Quat& a, float b);

    constexpr Quat& operator+=(Quat& a, const Quat& b);
    constexpr Quat& operator-=(Quat& a, const Quat& b);
    constexpr Quat& operator*=(Quat& a, const Quat& b);

    constexpr Quat& operator*=(Quat& a, float b);

    constexpr bool operator<(const Quat& a, const Quat& b);
    constexpr bool operator==(const Quat& a, const Quat& b);
    constexpr bool operator!=(const Quat& a, const Quat& b);
}

#include "he/math/inline/quat.inl"
