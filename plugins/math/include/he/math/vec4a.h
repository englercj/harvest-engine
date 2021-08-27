// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/utils.h"
#include "he/math/constants.h"
#include "he/math/float.h"
#include "he/math/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Construction

    Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y);
    Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y, const Vec4a& z);
    Vec4a MakeVec4a(const Vec4a& x, const Vec4a& y, const Vec4a& z, const Vec4a& w);

    template <typename T> Vec4a MakeVec4a(const Vec2T<T>& vec);
    template <typename T> Vec4a MakeVec4a(const Vec3T<T>& vec);
    template <typename T> Vec4a MakeVec4a(const Vec4T<T>& vec);

    template <typename T> Vec2T<T> MakeVec2(const Vec4a& v);
    template <typename T> Vec3T<T> MakeVec3(const Vec4a& v);
    template <typename T> Vec4T<T> MakeVec4(const Vec4a& v);

    // Returns a vector with all elements set to zero.
    Vec4a SplatZero();

    // Returns a vector with all elements set to `x`.
    Vec4a Splat(float x);

    // Returns a vector with all elements set to `v.x`.
    Vec4a SplatX(const Vec4a& v);

    // Returns a vector with all elements set to `v.y`.
    Vec4a SplatY(const Vec4a& v);

    // Returns a vector with all elements set to `v.z`.
    Vec4a SplatZ(const Vec4a& v);

    // Returns a vector with all elements set to `v.w`.
    Vec4a SplatW(const Vec4a& v);

    // --------------------------------------------------------------------------------------------
    // Component Access

    // Returns the value at index `i` in `v`.
    float GetComponent(const Vec4a& v, size_t i);

    // Returns a copy of `v` with the component at index `i` set to `s`.
    Vec4a SetComponent(const Vec4a& v, size_t i, float s);

    // Returns the X component of `v`.
    float GetX(const Vec4a& v);

    // Returns the Y component of `v`.
    float GetY(const Vec4a& v);

    // Returns the Z component of `v`.
    float GetZ(const Vec4a& v);

    // Returns the W component of `v`.
    float GetW(const Vec4a& v);

    // Sets the value of `v.x` to `s` and returns `v`.
    Vec4a SetX(const Vec4a& v, float s);

    // Sets the value of `v.y` to `s` and returns `v`.
    Vec4a SetY(const Vec4a& v, float s);

    // Sets the value of `v.z` to `s` and returns `v`.
    Vec4a SetZ(const Vec4a& v, float s);

    // Sets the value of `v.w` to `s` and returns `v`.
    Vec4a SetW(const Vec4a& v, float s);

    // Loads a vector from `p`, which must be 16-byte aligned.
    Vec4a Load(const float* p);

    // Loads a vector from `p`, which has no alignment requirements.
    Vec4a LoadU(const float* p);

    // Stores the values of `v` in `p`, which must be 16-byte aligned.
    void Store(float* p, const Vec4a& v);

    // Stores the values of `v` in `p`, which has no alignment requirements.
    void StoreU(float* p, const Vec4a& v);

    // --------------------------------------------------------------------------------------------
    // Classification

    // Returns true if any component of `v` is NaN.
    bool IsNan(const Vec4a& v);

    // Returns true if any of the first three components (x, y, z) of `v` are NaN.
    bool IsNan3(const Vec4a& v);

    // Returns true if any component of `v` is infinite and not NaN.
    bool IsInfinite(const Vec4a& v);

    // Returns true if any of the first three components (x, y, z) of `v` are infinite and not NaN.
    bool IsInfinite3(const Vec4a& v);

    // Returns true if all the components of `v` are not infite and not NaN.
    bool IsFinite(const Vec4a& v);

    // Returns true if all of the first three components (x, y, z) of `v` are not infite and not NaN.
    bool IsFinite3(const Vec4a& v);

    // Returns true if all the components of `v` are large enough for reciprocal operations.
    bool IsZeroSafe(const Vec4a& v);

    // Returns true if all of the first three components (x, y, z) of `v` are large enough for reciprocal operations.
    bool IsZeroSafe3(const Vec4a& v);

    // --------------------------------------------------------------------------------------------
    // Arithmetic

    // Returns the negation of each component of `v`.
    Vec4a Negate(const Vec4a& v);

    // Returns the component-wise addition of `a` and `b`.
    Vec4a Add(const Vec4a& a, const Vec4a& b);

    // Returns the component-wise addition of `a` and `b`.
    Vec4a Add(const Vec4a& a, float b);

    // Returns the component-wise subtraction of `b` from `a`.
    Vec4a Sub(const Vec4a& a, const Vec4a& b);

    // Returns the component-wise subtraction of `b` from `a`.
    Vec4a Sub(const Vec4a& a, float b);

    // Returns the component-wise multiplication of `a` with `b`.
    Vec4a Mul(const Vec4a& a, const Vec4a& b);

    // Returns the multiplication of each component of `a` with `b`.
    Vec4a Mul(const Vec4a& a, float b);

    // Multiplies `a` and `b` and adds the result to `add`.
    // Equivalent to `Add(add, Mul(a, b))` but potentially faster.
    Vec4a MulAdd(const Vec4a& a, const Vec4a& b, const Vec4a& add);

    // Returns the component-wise division of `a` by `b`.
    Vec4a Div(const Vec4a& a, const Vec4a& b);

    // Returns the division of each component of `a` by `b`.
    Vec4a Div(const Vec4a& a, float b);

    // Returns the component-wise linear interpolation between `a` and `b` by `t`.
    Vec4a Lerp(const Vec4a& a, const Vec4a& b, float t);

    // Performs smooth Hermite interpolation between 0 and 1 when `a` < `t` < `b`.
    Vec4a SmoothStep(const Vec4a& a, const Vec4a& b, const Vec4a& t);

    // Returns the absolute value for each component of `v`.
    Vec4a Abs(const Vec4a& v);

    // Returns the component-wise reciprocal of `v`.
    Vec4a Rcp(const Vec4a& v);

    // Returns the component-wise reciprocal of `v` or 0 for small values.
    Vec4a RcpSafe(const Vec4a& v);

    // Returns the component-wise square root of `v`.
    Vec4a Sqrt(const Vec4a& v);

    // Returns component-wise reciprocal square root of `v`.
    Vec4a Rsqrt(const Vec4a& v);

    // --------------------------------------------------------------------------------------------
    // Selection

    // Returns the smaller value between `x` and `y`.
    Vec4a Min(const Vec4a& a, const Vec4a& b);

    // Returns the larger value between `x` and `y`.
    Vec4a Max(const Vec4a& a, const Vec4a& b);

    // Returns a clamped value between `lo` and `hi`.
    Vec4a Clamp(const Vec4a& v, const Vec4a& lo, const Vec4a& hi);

    // Returns a the smallest component of `v`.
    float HMin(const Vec4a& v);

    // Returns a the largest component of `v`.
    float HMax(const Vec4a& v);

    // Returns a the sum of all components of `v`.
    float HAdd(const Vec4a& v);

    // --------------------------------------------------------------------------------------------
    // Geometric

    // Returns the dot product of `a` and `b`.
    float Dot(const Vec4a& a, const Vec4a& b);

    // Returns the dot product of the first three components (x, y, z) of `a` and `b`.
    float Dot3(const Vec4a& a, const Vec4a& b);

    // Returns the squared-length of `v`.
    float LenSquared(const Vec4a& v);

    // Returns the squared-length of the first three components (x, y, z) of `v`.
    float LenSquared3(const Vec4a& v);

    // Returns the length of `v`.
    float Len(const Vec4a& v);

    // Returns the length of the first three components (x, y, z) of `v`.
    float Len3(const Vec4a& v);

    // Returns the cross product `a` and `b`.
    Vec4a Cross(const Vec4a& a, const Vec4a& b);

    // Returns a normalized vector from `v`.
    Vec4a Normalize(const Vec4a& v);

    // Returns a normalized vector of the first three components (x, y, z) from `v`.
    Vec4a Normalize3(const Vec4a& v);

    // Returns true if `v` is a normalized vector.
    bool IsNormalized(const Vec4a& v);

    // Returns true if the first three components (x, y, z) of `v` are a normalized vector.
    bool IsNormalized3(const Vec4a& v);

    // --------------------------------------------------------------------------------------------
    // Comparison

    // Returns the component-wise test of `a < b`.
    Vec4a Lt(const Vec4a& a, const Vec4a& b);

    // Returns the component-wise test of `a <= b`.
    Vec4a Le(const Vec4a& a, const Vec4a& b);

    // Returns the component-wise test of `a > b`.
    Vec4a Gt(const Vec4a& a, const Vec4a& b);

    // Returns the component-wise test of `a >= b`.
    Vec4a Ge(const Vec4a& a, const Vec4a& b);

    // Returns the component-wise test of `a == b`.
    Vec4a Eq(const Vec4a& a, const Vec4a& b);

    // Returns the component-wise test of `a != b`.
    Vec4a Ne(const Vec4a& a, const Vec4a& b);

    // Returns the component-wise test of `a == b`, treating the components like integers.
    // Since nan != nan this can sometimes be necessary to check if values are bitwise equivalent.
    Vec4a EqInt(const Vec4a& a, const Vec4a& b);

    // Returns true if any elements in `cmp` are nonzero.
    bool Any(const Vec4a& cmp);

    // Returns true if any of the first three elements in `cmp` are nonzero.
    bool Any3(const Vec4a& cmp);

    // Returns true if all of the elements in `cmp` are nonzero.
    bool All(const Vec4a& cmp);

    // Returns true if all of the first three elements in `cmp` are nonzero.
    bool All3(const Vec4a& cmp);

    // --------------------------------------------------------------------------------------------
    // Vector operator overloads

    Vec4a operator-(const Vec4a& v);

    Vec4a operator+(const Vec4a& a, const Vec4a& b);
    Vec4a operator-(const Vec4a& a, const Vec4a& b);
    Vec4a operator*(const Vec4a& a, const Vec4a& b);
    Vec4a operator/(const Vec4a& a, const Vec4a& b);

    Vec4a operator+(const Vec4a& a, float b);
    Vec4a operator-(const Vec4a& a, float b);
    Vec4a operator*(const Vec4a& a, float b);
    Vec4a operator/(const Vec4a& a, float b);

    Vec4a& operator+=(Vec4a& a, const Vec4a& b);
    Vec4a& operator-=(Vec4a& a, const Vec4a& b);
    Vec4a& operator*=(Vec4a& a, const Vec4a& b);
    Vec4a& operator/=(Vec4a& a, const Vec4a& b);

    Vec4a& operator+=(Vec4a& a, float b);
    Vec4a& operator-=(Vec4a& a, float b);
    Vec4a& operator*=(Vec4a& a, float b);
    Vec4a& operator/=(Vec4a& a, float b);

    bool operator<(const Vec4a& a, const Vec4a& b);
    bool operator==(const Vec4a& a, const Vec4a& b);
    bool operator!=(const Vec4a& a, const Vec4a& b);
}

#include "he/math/inline/vec4a.inl"

#if HE_SIMD_SSE2
    #include "he/math/inline/vec4a.sse.inl"
#elif HE_SIMD_NEON
    #include "he/math/inline/vec4a.neon.inl"
#else
    #include "he/math/inline/vec4a.seq.inl"
#endif
