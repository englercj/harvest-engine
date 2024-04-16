// Copyright Chad Engler

#pragma once

#include "he/core/limits.h"
#include "he/core/types.h"

namespace he
{
    /// A collection of mathematical constants.
    template <typename T>
    struct MathConstants
    {
        /// The value of e.
        static constexpr T E = T(2.71828182845904523536);

        /// The value of log_2(e).
        static constexpr T Log2E = T(1.44269504088896340736);

        /// The value of log_10(e).
        static constexpr T Log10E = T(0.434294481903251827651);

        /// The value of log_e(2) aka ln(2).
        static constexpr T LogE2 = T(0.693147180559945309417);

        /// The value of log_e(10) aka ln(10).
        static constexpr T LogE10 = T(2.30258509299404568402);

        /// The value of pi.
        static constexpr T Pi = T(3.14159265358979323846);

        /// Twice the value of pi.
        static constexpr T Pi2 = T(6.28318530717958647692);

        /// Half the value of pi.
        static constexpr T PiHalf = T(1.57079632679489661923);

        /// A quarter of the value of pi.
        static constexpr T PiQuarter = T(0.78539816339744830962);

        /// The square root of 2.
        static constexpr T Sqrt2 = T(1.41421356237309504880);

        /// The reciprical of the square root of 2.
        static constexpr T Sqrt2Rcp = T(0.70710678118654746);
    };

    /// The floating-point classification of a value.
    enum class FpClass
    {
        Normal,     ///< A normal value.
        Subnormal,  ///< A subnormal value.
        Zero,       ///< A zero value.
        Infinite,   ///< An infinite value.
        Nan,        ///< A NaN value.
    };

    /// Returns the floating-point classification of the parameter.
    ///
    /// \param x The value to classify.
    /// \return The classification of `x`.
    constexpr FpClass Classify(float x) noexcept;

    /// \copydoc Classify(float)
    constexpr FpClass Classify(double x) noexcept;

    /// \copydoc Classify(float)
    constexpr FpClass Classify(long double x) noexcept;

    /// Tests if the parameter is NaN.
    ///
    /// \param x The value to check.
    /// \return True if the value is NaN; false otherwise.
    constexpr bool IsNan(float x) noexcept;

    /// \copydoc IsNan(float)
    constexpr bool IsNan(double x) noexcept;

    /// \copydoc IsNan(float)
    constexpr bool IsNan(long double x) noexcept;

    /// Tests if the parameter is infinite and is not NaN.
    ///
    /// \param x The value to check.
    /// \return True if the value is infinite and not NaN; false otherwise.
    constexpr bool IsInfinite(float x) noexcept;

    /// \copydoc IsInfinite(float)
    constexpr bool IsInfinite(double x) noexcept;

    /// \copydoc IsInfinite(float)
    constexpr bool IsInfinite(long double x) noexcept;

    /// Tests if the parameter is not infinite and is not NaN.
    ///
    /// \param x The value to check.
    /// \return True if the value is not infinite and not NaN; false otherwise.
    constexpr bool IsFinite(float x) noexcept;

    /// \copydoc IsFinite(float)
    constexpr bool IsFinite(double x) noexcept;

    /// \copydoc IsFinite(float)
    constexpr bool IsFinite(long double x) noexcept;

    /// Tests if the parameter is normal, i.e. not zero, subnormal, infinite, or NaN.
    ///
    /// \param x The value to check.
    /// \return True if the value is not zero, subnormal, infinite, or NaN; false otherwise.
    constexpr bool IsNormal(float x) noexcept;

    /// \copydoc IsNormal(float)
    constexpr bool IsNormal(double x) noexcept;

    /// \copydoc IsNormal(float)
    constexpr bool IsNormal(long double x) noexcept;

    /// Tests if the parameter is a large enough value for reciprocal operations.
    ///
    /// \param x The value to check.
    /// \return True if the value is safe to use as a denominator.
    constexpr bool IsZeroSafe(float x) noexcept;

    /// \copydoc IsZeroSafe(float)
    constexpr bool IsZeroSafe(double x) noexcept;

    /// \copydoc IsZeroSafe(float)
    constexpr bool IsZeroSafe(long double x) noexcept;

    /// Tests if two floating-point values are nearly equal given some tolerance.
    ///
    /// \param a The first floating point value to compare.
    /// \param b The second floating point value to compare.
    /// \param tolerance The maximum difference between the values that is considered equal.
    /// \return True if the values are nearly equal; false otherwise.
    constexpr bool IsNearlyEqual(float a, float b, float tolerance = Limits<float>::Epsilon) noexcept;

    /// \copydoc IsNearlyEqual(float, float, float)
    constexpr bool IsNearlyEqual(double a, double b, double tolerance = Limits<double>::Epsilon) noexcept;

    /// \copydoc IsNearlyEqual(float, float, float)
    constexpr bool IsNearlyEqual(long double a, long double b, long double tolerance = Limits<long double>::Epsilon) noexcept;

    /// Tests if two floating-point values are nearly equal given some ULP tolerance.
    ///
    /// Based on: https://www.gamasutra.com/view/news/162368/Indepth_Comparing_floating_point_numbers_2012_edition.php
    ///
    /// \param a The first floating point value to compare.
    /// \param b The second floating point value to compare.
    /// \param maxUlpDiff The maximum tolerance for floating point ULP diff.
    /// \return True if the values are nearly equal; false otherwise.
    constexpr bool IsNearlyEqualULP(float a, float b, uint32_t maxUlpDiff) noexcept;

    /// \copydoc IsNearlyEqualULP(float, float, uint32_t)
    constexpr bool IsNearlyEqualULP(double a, double b, uint32_t maxUlpDiff) noexcept;

    /// \copydoc IsNearlyEqualULP(float, float, uint32_t)
    constexpr bool IsNearlyEqualULP(long double a, long double b, uint32_t maxUlpDiff) noexcept;

    /// Tests if the given floating point number is negative.
    ///
    /// \param x The value to check.
    /// \return True if the value is negative; false otherwise.
    constexpr bool HasSignBit(float x) noexcept;

    /// \copydoc HasSignBit(float)
    constexpr bool HasSignBit(double x) noexcept;

    /// \copydoc HasSignBit(float)
    constexpr bool HasSignBit(long double x) noexcept;

    /// Returns the absolute value of the parameter.
    ///
    /// \param x The value to get the absolute value of.
    /// \return The absolute value of `x`.
    constexpr float Abs(float x) noexcept;

    /// \copydoc Abs(float)
    constexpr double Abs(double x) noexcept;

    /// \copydoc Abs(float)
    constexpr long double Abs(long double x) noexcept;

    /// Returns the minimum of the two parameters.
    ///
    /// \param x The first value to compare.
    /// \param y The second value to compare.
    /// \return The minimum of `x` and `y`.
    constexpr float Min(float x, float y) noexcept;

    /// \copydoc Min(float, float)
    constexpr double Min(double x, double y) noexcept;

    /// \copydoc Min(float, float)
    constexpr long double Min(long double x, long double y) noexcept;

    /// Returns the maximum of the two parameters.
    ///
    /// \param x The first value to compare.
    /// \param y The second value to compare.
    /// \return The maximum of `x` and `y`.
    constexpr float Max(float x, float y) noexcept;

    /// \copydoc Max(float, float)
    constexpr double Max(double x, double y) noexcept;

    /// \copydoc Max(float, float)
    constexpr long double Max(long double x, long double y) noexcept;

    /// Floors the parameter down to the previous integer value.
    ///
    /// \param x The value to floor.
    /// \return The closest integer that is less than or equal to the input value.
    constexpr float Floor(float x) noexcept;

    /// \copydoc Floor(float)
    constexpr double Floor(double x) noexcept;

    /// \copydoc Floor(float)
    constexpr long double Floor(long double x) noexcept;

    /// Ceilings the parameter up to the next integer value.
    ///
    /// \param x The value to ceiling.
    /// \return The closest integer that is greater than or equal to the input value.
    constexpr float Ceil(float x) noexcept;

    /// \copydoc Ceil(float)
    constexpr double Ceil(double x) noexcept;

    /// \copydoc Ceil(float)
    constexpr long double Ceil(long double x) noexcept;

    /// Rounds the parameter to the closest integer value.
    ///
    /// \param x The value to round.
    /// \return The closest integer to the input value.
    constexpr float Round(float x) noexcept;

    /// \copydoc Round(float)
    constexpr double Round(double x) noexcept;

    /// \copydoc Round(float)
    constexpr long double Round(long double x) noexcept;

    /// Converts the parameter to radians.
    ///
    /// \param deg The angle as degrees.
    /// \return The angle as radians.
    constexpr float ToRadians(float deg) noexcept;

    /// \copydoc ToRadians(float)
    constexpr double ToRadians(double deg) noexcept;

    /// \copydoc ToRadians(float)
    constexpr long double ToRadians(long double deg) noexcept;

    /// Convert the parameter to degrees.
    ///
    /// \param rad The angle as radians.
    /// \return The angle as degrees.
    constexpr float ToDegrees(float rad) noexcept;

    /// \copydoc ToDegrees(float)
    constexpr double ToDegrees(double rad) noexcept;

    /// \copydoc ToDegrees(float)
    constexpr long double ToDegrees(long double rad) noexcept;

    /// Linearly interpolates from `a` to `b` by `t`, which is in the range [0,1].
    ///
    /// \param a The starting value for the interpolation, i.e. the value when `t == 0`.
    /// \param b The ending value for the interpolation, i.e. the value when `t == 1`.
    /// \param t The "time step" for the interpolation. Expected to be in the range `[0, 1]`.
    /// \return The interpolated value.
    constexpr float Lerp(float a, float b, float t) noexcept;

    /// \copydoc Lerp(float, float, float)
    constexpr double Lerp(double a, double b, double t) noexcept;

    /// \copydoc Lerp(float, float, float)
    constexpr long double Lerp(long double a, long double b, long double t) noexcept;

    /// Performs smooth Hermite interpolation between `0` and `1`
    ///
    /// \param a The left edge of the curve, i.e. the value that returns `0` when `t <= a`.
    /// \param b The right edge of the curve, i.e. the value that returns `1` when `t >= b`.
    /// \param t The step along the curve to interpolate.
    /// \return The interpolated value.
    constexpr float SmoothStep(float a, float b, float t) noexcept;

    /// \copydoc SmoothStep(float, float, float)
    constexpr double SmoothStep(double a, double b, double t) noexcept;

    /// \copydoc SmoothStep(float, float, float)
    constexpr long double SmoothStep(long double a, long double b, long double t) noexcept;

    /// Calculates the reciprocal of the parameter.
    ///
    /// \param x The value to
    /// \return The reciprocal of `x`.
    constexpr float Rcp(float x) noexcept;

    /// \copydoc Rcp(float)
    constexpr double Rcp(double x) noexcept;

    /// \copydoc Rcp(float)
    constexpr long double Rcp(long double x) noexcept;

    /// Calculates the reciprocal of the parameter or `0` for very small values.
    ///
    /// \param x The value to
    /// \return The reciprocal of `x`.
    constexpr float RcpSafe(float x) noexcept;

    /// \copydoc RcpSafe(float)
    constexpr double RcpSafe(double x) noexcept;

    /// \copydoc RcpSafe(float)
    constexpr long double RcpSafe(long double x) noexcept;

    /// Calculates the square root of the parameter.
    ///
    /// \param x The value to sqrt.
    /// \return The square root of `x`.
    float Sqrt(float x) noexcept;

    /// \copydoc Sqrt(float)
    double Sqrt(double x) noexcept;

    /// Calculates the reciprocal of the square root of `x`.
    ///
    /// \param x The value to sqrt.
    /// \return The square root of `x`.
    float Rsqrt(float x) noexcept;

    /// \copydoc Rsqrt(float)
    double Rsqrt(double x) noexcept;

    /// Calculates the sine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The sine of `x`.
    float Sin(float x) noexcept;

    /// \copydoc Sin(float)
    double Sin(double x) noexcept;

    /// Calculates the arc sine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The arc sine of `x`.
    float Asin(float x) noexcept;

    /// \copydoc Asin(float)
    double Asin(double x) noexcept;

    /// Calculates the cosine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The cosine of `x`.
    float Cos(float x) noexcept;

    /// \copydoc Cos(float)
    double Cos(double x) noexcept;

    /// Calculates the arc cosine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The arc cosine of `x`.
    float Acos(float x) noexcept;

    /// \copydoc Acos(float)
    double Acos(double x) noexcept;

    /// Calculates the tangent of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The tangent of `x`.
    float Tan(float x) noexcept;

    /// \copydoc Tan(float)
    double Tan(double x) noexcept;

    /// Calculates the arc tangent of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \return The arc tangent of `x`.
    float Atan(float x) noexcept;

    /// \copydoc Atan(float)
    double Atan(double x) noexcept;

    /// Calculates the arc tangent of `y/x` using the signs of arguments to determine the correct quadrant.
    ///
    /// \param y The Y value.
    /// \param x The X value.
    /// \return The arc tangent of `y/x` in the range `[-pi, pi]`.
    float Atan2(float y, float x) noexcept;

    /// \copydoc Atan2(float, float)
    double Atan2(double y, double x) noexcept;

    /// Calculates the sine and cosine of the parameter, in radians.
    ///
    /// \param x The angle in radians.
    /// \param[out] s The sine of `x`.
    /// \param[out] c The cosine of `x`.
    void SinCos(float x, float& s, float& c) noexcept;

    /// \copydoc SinCos(float, float&, float&)
    void SinCos(double x, double& s, double& c) noexcept;

    /// Calculates `e` (Euler's number) raised to the power of the parameter.
    ///
    /// \param x The exponent to raise `e` to.
    /// \return The base-e exponential of `x` (e^x)
    float Exp(float x) noexcept;

    /// \copydoc Exp(float)
    double Exp(double x) noexcept;

    /// Calculates the natural (base-e) logarithm of the parameter.
    ///
    /// \param x The value to calculate the logarithm of.
    /// \return The natural (base-e) logarithm of `x`.
    float Log(float x) noexcept;

    /// \copydoc Log(float)
    double Log(double x) noexcept;

    /// Calculates the binary (base-2) logarithm of the parameter.
    ///
    /// \param x The value to calculate the logarithm of.
    /// \return The base-2 logarithm of `x`.
    float Log2(float x) noexcept;

    /// \copydoc Log2(float)
    double Log2(double x) noexcept;

    /// Calculates the common (base-10) logarithm of the parameter.
    ///
    /// \param x The value to calculate the logarithm of.
    /// \return The base-10 logarithm of `x`.
    float Log10(float x) noexcept;

    /// \copydoc Log2(float)
    double Log10(double x) noexcept;

    /// Calculates the natural (base-e) logarithm of one plus the parameter.
    /// This is more accurate than `Log(1 + x)` for small values of `x`.
    ///
    /// \param x The value to calculate the logarithm of.
    /// \return The base-e logarithm of `1 + x`.
    float Log1p(float x) noexcept;

    /// \copydoc Log1p(float)
    double Log1p(double x) noexcept;

    /// Calculates the base-`n` logarithm of the parameter.
    ///
    /// \param x The value to calculate the logarithm of.
    /// \param n The logarithm base.
    /// \return The base-`n` logarithm of `x`.
    float LogN(float x, float n) noexcept;

    /// \copydoc LogN(float, float)
    double LogN(double x, double n) noexcept;

    /// Calculates the value of base raised to the power exponent.
    ///
    /// \param base The base value.
    /// \param exp The exponent value.
    /// \return The `base` value raised to the power of `exp`.
    float Pow(float base, float exp) noexcept;

    /// \copydoc Pow(float, float)
    double Pow(double base, double exp) noexcept;

    /// Calculates the remainder of the division operation `x / y`.
    ///
    /// \param x The numerator.
    /// \param y The denominator.
    /// \return The remainder of the divison.
    float Fmod(float x, float y) noexcept;

    /// \copydoc Fmod(float, float)
    double Fmod(double x, double y) noexcept;
}

#include "he/core/inline/math.inl"
