// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/math/constants.h"
#include "he/math/float.h"
#include "he/math/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Construction

    // Converts `v` to other vector representations.
    template <typename T1, typename T2> constexpr Vec4<T1> MakeVec4(const Vec2<T2>& v, typename Vec2<T2>::Type z = 0, typename Vec2<T2>::Type w = 0);
    template <typename T1, typename T2> constexpr Vec4<T1> MakeVec4(const Vec3<T2>& v, typename Vec2<T2>::Type w = 0);
    template <typename T1, typename T2> constexpr Vec4<T1> MakeVec4(const Vec4<T2>& v);

    // --------------------------------------------------------------------------------------------
    // Component Access

    // Returns the value at index `i` in `v`.
    template <typename T> constexpr T GetComponent(const Vec4<T>& v, size_t i);

    // Sets the value at index `i` in `v` to `s` and returns `v`.
    template <typename T> constexpr Vec4<T>& SetComponent(Vec4<T>& v, size_t i, T s);

    // Returns a pointer to the vector data.
    template <typename T> constexpr T* GetPointer(Vec4<T>& v);

    // Returns a const pointer to the vector data.
    template <typename T> constexpr const T* GetPointer(const Vec4<T>& v);

    // --------------------------------------------------------------------------------------------
    // Classification

    // Returns true if any component of `v` is NaN.
    template <typename T> constexpr bool IsNan(const Vec4<T>& v);

    // Returns true if any component of `v` is infinite and not NaN.
    template <typename T> constexpr bool IsInfinite(const Vec4<T>& v);

    // Returns true if all the components of `v` are not infite and not NaN.
    template <typename T> constexpr bool IsFinite(const Vec4<T>& v);

    // Returns true if all the components of `v` are large enough for reciprocal operations.
    template <typename T> constexpr bool IsZeroSafe(const Vec4<T>& v);

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    // Returns the negation of each component of `v`.
    template <typename T> constexpr Vec4<T> Negate(const Vec4<T>& v);

    // Returns the component-wise addition of `a` and `b`.
    template <typename T> constexpr Vec4<T> Add(const Vec4<T>& a, const Vec4<T>& b);

    // Returns the component-wise addition of `a` and `b`.
    template <typename T> constexpr Vec4<T> Add(const Vec4<T>& a, T b);

    // Returns the component-wise addition of `a` and `b`.
    template <typename T> constexpr Vec4<T> Add(const Vec4<T>& a, const Vec4<T>& b);

    // Returns the component-wise addition of `a` and `b`.
    template <typename T> constexpr Vec4<T> Add(const Vec4<T>& a, T b);

    // Returns the component-wise multiplication of `a` with `b`.
    template <typename T> constexpr Vec4<T> Mul(const Vec4<T>& a, const Vec4<T>& b);

    // Returns the multiplication of each component of `a` with `b`.
    template <typename T> constexpr Vec4<T> Mul(const Vec4<T>& a, T b);

    // Returns the component-wise division of `a` by `b`.
    template <typename T> constexpr Vec4<T> Div(const Vec4<T>& a, const Vec4<T>& b);

    // Returns the division of each component of `a` by `b`.
    template <typename T> constexpr Vec4<T> Div(const Vec4<T>& a, T b);

    // Returns the component-wise multiplication of `a` and `b` added to `c`.
    template <typename T> constexpr Vec4<T> MulAdd(const Vec4<T>& a, const Vec4<T>& b, const Vec4<T>& c);

    // Returns the component-wise linear interpolation between `a` and `b` by `t`.
    template <typename T> constexpr Vec4<T> Lerp(const Vec4<T>& a, const Vec4<T>& b, float t);

    // Performs smooth Hermite interpolation between 0 and 1 when `a` < `t` < `b`.
    template <typename T> constexpr Vec4<T> SmoothStep(float a, float b, const Vec4<T>& t);

    // Returns the absolute value for each component of `v`.
    template <typename T> constexpr Vec4<T> Abs(const Vec4<T>& v);

    // Returns the component-wise reciprocal of `v`.
    template <typename T> constexpr Vec4<T> Rcp(const Vec4<T>& v);

    // Returns the component-wise reciprocal of `v` or 0 for small values.
    template <typename T> constexpr Vec4<T> RcpSafe(const Vec4<T>& v);

    // Returns the component-wise square root of `v`.
    template <typename T> Vec4<T> Sqrt(const Vec4<T>& v);

    // Returns the component-wise reciprocal square root of `v`.
    template <typename T> Vec4<T> Rsqrt(const Vec4<T>& v);

    // --------------------------------------------------------------------------------------------
    // Selection

    // Returns the smaller value between `x` and `y`.
    template <typename T> constexpr Vec4<T> Min(const Vec4<T>& a, const Vec4<T>& b);

    // Returns the larger value between `x` and `y`.
    template <typename T> constexpr Vec4<T> Max(const Vec4<T>& a, const Vec4<T>& b);

    // Returns a clamped value between `lo` and `hi`.
    template <typename T> constexpr Vec4<T> Clamp(const Vec4<T>& v, const Vec4<T>& lo, const Vec4<T>& hi);

    // Returns a vector with all components set to the smallest component of `v`.
    template <typename T> constexpr T HMin(const Vec4<T>& v);

    // Returns a vector with all components set to the largest component of `v`.
    template <typename T> constexpr T HMax(const Vec4<T>& v);

    // Returns a vector with all components set to the sum of all components of `v`.
    // Order of operations is (x + y)
    template <typename T> constexpr T HAdd(const Vec4<T>& v);

    // --------------------------------------------------------------------------------------------
    // Geometric

    // Returns the dot product of `a` and `b`.
    template <typename T> constexpr T Dot(const Vec4<T>& a, const Vec4<T>& b);

    // Returns the squared-length of the `v`.
    template <typename T> constexpr T LenSquared(const Vec4<T>& v);

    // Returns the length of `v`.
    template <typename T> T Len(const Vec4<T>& v);

    // Returns a normalized vector from `v`.
    template <typename T> Vec4<T> Normalize(const Vec4<T>& v);

    // Returns a normalized vector from `v` or `v` if the vector cannot be normalized.
    template <typename T> Vec4<T> NormalizeSafe(const Vec4<T>& v);

    // Returns true if `v` is a normalized vector.
    template <typename T> bool IsNormalized(const Vec4<T>& v);

    // --------------------------------------------------------------------------------------------
    // Operators

    template <typename T> constexpr Vec4<T> operator-(const Vec4<T>& v);

    template <typename T> constexpr Vec4<T> operator+(const Vec4<T>& a, const Vec4<T>& b);
    template <typename T> constexpr Vec4<T> operator-(const Vec4<T>& a, const Vec4<T>& b);
    template <typename T> constexpr Vec4<T> operator*(const Vec4<T>& a, const Vec4<T>& b);
    template <typename T> constexpr Vec4<T> operator/(const Vec4<T>& a, const Vec4<T>& b);

    template <typename T> constexpr Vec4<T> operator+(const Vec4<T>& a, T b);
    template <typename T> constexpr Vec4<T> operator-(const Vec4<T>& a, T b);
    template <typename T> constexpr Vec4<T> operator*(const Vec4<T>& a, T b);
    template <typename T> constexpr Vec4<T> operator/(const Vec4<T>& a, T b);

    template <typename T> constexpr Vec4<T>& operator+=(Vec4<T>& a, const Vec4<T>& b);
    template <typename T> constexpr Vec4<T>& operator-=(Vec4<T>& a, const Vec4<T>& b);
    template <typename T> constexpr Vec4<T>& operator*=(Vec4<T>& a, const Vec4<T>& b);
    template <typename T> constexpr Vec4<T>& operator/=(Vec4<T>& a, const Vec4<T>& b);

    template <typename T> constexpr Vec4<T>& operator+=(Vec4<T>& a, T b);
    template <typename T> constexpr Vec4<T>& operator-=(Vec4<T>& a, T b);
    template <typename T> constexpr Vec4<T>& operator*=(Vec4<T>& a, T b);
    template <typename T> constexpr Vec4<T>& operator/=(Vec4<T>& a, T b);

    template <typename T> constexpr bool operator<(const Vec4<T>& a, const Vec4<T>& b);
    template <typename T> constexpr bool operator==(const Vec4<T>& a, const Vec4<T>& b);
    template <typename T> constexpr bool operator!=(const Vec4<T>& a, const Vec4<T>& b);
}

#include "he/math/inline/vec4.inl"
