// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/math/constants.h"
#include "he/math/types.h"

#include <cmath>

namespace he
{
    /// Tests if the parameter is NaN.
    ///
    /// \param x The value to check.
    /// \return True if the value is NaN, false otherwise.
    constexpr bool IsNan(float x) noexcept;

    /// Tests if the parameter is infinite and is not NaN.
    ///
    /// \param x The value to check.
    /// \return True if the value is infinite and not NaN, false otherwise.
    constexpr bool IsInfinite(float x) noexcept;

    /// Tests if the parameter is not infinite and is not NaN.
    ///
    /// \param x The value to check.
    /// \return True if the value is not infinite and not NaN, false otherwise.
    constexpr bool IsFinite(float x) noexcept;

    /// Tests if the parameter is a large enough value for reciprocal operations.
    ///
    /// \param x The value to check.
    /// \return True if the value is safe to use as a denominator.
    constexpr bool IsZeroSafe(float x) noexcept;

    /// Floors the parameter down to the previous integer value.
    ///
    /// \param x The value to floor.
    /// \return The closest integer that is less than or equal to the input value.
    constexpr float Floor(float x) noexcept;

    /// Ceilings the parameter up to the next integer value.
    ///
    /// \param x The value to ceiling.
    /// \return The closest integer that is greater than or equal to the input value.
    constexpr float Ceil(float x) noexcept;

    /// Rounds the parameter to the closest integer value.
    ///
    /// \param x The value to round.
    /// \return The closest integer to the input value.
    constexpr float Round(float x) noexcept;

    /// Converts the parameter to radians.
    ///
    /// \param deg The angle as degrees.
    /// \return The angle as radians.
    constexpr float ToRadians(float deg) noexcept;

    /// Convert the parameter to degrees.
    ///
    /// \param rad The angle as radians.
    /// \return The angle as degrees.
    constexpr float ToDegrees(float rad) noexcept;

    /// Linearly interpolates from `a` to `b` by `t`, which is in the range [0,1].
    ///
    /// \param a The starting value for the interpolation, i.e. the value when `t == 0`.
    /// \param b The ending value for the interpolation, i.e. the value when `t == 1`.
    /// \param t The "time step" for the interpolation. Expected to be in the range `[0, 1]`.
    /// \return The interpolated value.
    constexpr float Lerp(float a, float b, float t) noexcept;

    /// Performs smooth Hermite interpolation between `0` and `1`
    ///
    /// \param a The left edge of the curve, i.e. the value that returns `0` when `t <= a`.
    /// \param b The right edge of the curve, i.e. the value that returns `1` when `t >= b`.
    /// \param t The step along the curve to interpolate.
    /// \return The interpolated value.
    constexpr float SmoothStep(float a, float b, float t) noexcept;

    /// Calculates the reciprocal of the parameter.
    ///
    /// \param x The value to
    /// \return The reciprocal of `x`.
    constexpr float Rcp(float x) noexcept;

    /// Calculates the reciprocal of the parameter or `0` for very small values.
    ///
    /// \param x The value to
    /// \return The reciprocal of `x`.
    constexpr float RcpSafe(float x) noexcept;

    /// Calculates the square root of the parameter.
    ///
    /// \param x The value to sqrt.
    /// \return The square root of `x`.
    float Sqrt(float x) noexcept;

    /// Calculates the reciprocal of the square root of `x`.
    ///
    /// \param x The value to sqrt.
    /// \return The square root of `x`.
    float Rsqrt(float x) noexcept;

    /// Calculates the sine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The sine of `x`.
    float Sin(float x) noexcept;

    /// Calculates the arc sine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The arc sine of `x`.
    float Asin(float x) noexcept;

    /// Calculates the cosine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The cosine of `x`.
    float Cos(float x) noexcept;

    /// Calculates the arc cosine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The arc cosine of `x`.
    float Acos(float x) noexcept;

    /// Calculates the tangent of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The tangent of `x`.
    float Tan(float x) noexcept;

    /// Calculates the arc tangent of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The arc tangent of `x`.
    float Atan(float x) noexcept;

    /// Calculates the arc tangent of `y/x` using the signs of arguments to determine the correct quadrant.
    ///
    /// \param y The Y value.
    /// \param x The X value.
    /// \return The arc tangent of `y/x` in the range `[-pi, pi]`.
    float Atan2(float y, float x) noexcept;

    /// Calculates the sine and cosine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \param[out] s The sine of `x`.
    /// \param[out] c The cosine of `x`.
    void SinCos(float x, float& s, float& c) noexcept;

    /// Calculates `e` (Euler's number) raised to the power of the parameter.
    ///
    /// \param x The exponent to raise `e` to.
    /// \return The base-e exponential of `x` (e^x)
    float Exp(float x) noexcept;

    /// Calculates the natural logarithm (base-e) of the parameter.
    ///
    /// \param x The value to calculate the logarithm of.
    /// \return The natural (base-e) logarithm of `x`.
    float Ln(float x) noexcept;

    /// Calculates the base-2 logarithm of the parameter.
    ///
    /// \param x The value to calculate the logarithm of.
    /// \return The base-2 logarithm of `x`.
    float Lb(float x) noexcept;

    /// Calculates the base-`n` logarithm of the parameter.
    ///
    /// \param x The value to calculate the logarithm of.
    /// \param n The logarithm base.
    /// \return The base-`n` logarithm of `x`.
    float LogN(float x, float n) noexcept;

    /// Calculates the value of base raised to the power exponent.
    ///
    /// \param base The base value.
    /// \param exp The exponent value.
    /// \return The `base` value raised to the power of `exp`.
    float Pow(float base, float exp) noexcept;

    /// Calculates the remainder of the division operation `x / y`.
    ///
    /// \param x The numerator.
    /// \param y The denominator.
    /// \return The remainder of the divison.
    float Fmod(float x, float y) noexcept;
}

#include "he/math/inline/float.inl"
