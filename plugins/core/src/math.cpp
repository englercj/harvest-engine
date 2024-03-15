// Copyright Chad Engler

#include "he/core/math.h"

#include "he/core/compiler.h"
#include "he/core/simd.h"

#include <cmath>

namespace he
{
    // TODO: Remove libc usage and do SSE/NEON implementations of some of these trig functions.
    // For example: http://gruntthepeon.free.fr/ssemath/

    // --------------------------------------------------------------------------------------------
    float Sin(float x) noexcept
    {
        return std::sinf(x);
    }

    double Sin(double x) noexcept
    {
        return std::sin(x);
    }

    // --------------------------------------------------------------------------------------------
    float Asin(float x) noexcept
    {
        return std::asinf(x);
    }

    double Asin(double x) noexcept
    {
        return std::asin(x);
    }

    // --------------------------------------------------------------------------------------------
    float Cos(float x) noexcept
    {
        return std::cosf(x);
    }

    double Cos(double x) noexcept
    {
        return std::cos(x);
    }

    // --------------------------------------------------------------------------------------------
    float Acos(float x) noexcept
    {
        return std::acosf(x);
    }

    double Acos(double x) noexcept
    {
        return std::acos(x);
    }

    // --------------------------------------------------------------------------------------------
    float Tan(float x) noexcept
    {
        return std::tanf(x);
    }

    double Tan(double x) noexcept
    {
        return std::tan(x);
    }

    // --------------------------------------------------------------------------------------------
    float Atan(float x) noexcept
    {
        return std::atanf(x);
    }

    double Atan(double x) noexcept
    {
        return std::atan(x);
    }

    // --------------------------------------------------------------------------------------------
    float Atan2(float y, float x) noexcept
    {
        return std::atan2f(y, x);
    }

    double Atan2(double y, float x) noexcept
    {
        return std::atan2(y, x);
    }

    // --------------------------------------------------------------------------------------------
    void SinCos(float x, float& s, float& c) noexcept
    {
        // TODO: Optimize
        s = Sin(x);
        c = Cos(x);
    }

    void SinCos(double x, double& s, double& c) noexcept
    {
        // TODO: Optimize
        s = Sin(x);
        c = Cos(x);
    }

    // --------------------------------------------------------------------------------------------
    float Exp(float x) noexcept
    {
        return std::expf(x);
    }

    double Exp(double x) noexcept
    {
        return std::exp(x);
    }

    // --------------------------------------------------------------------------------------------
    float Ln(float x) noexcept
    {
        return std::logf(x);
    }

    double Ln(double x) noexcept
    {
        return std::log(x);
    }

    // --------------------------------------------------------------------------------------------
    float Lb(float x) noexcept
    {
        return std::log2f(x);
    }

    double Lb(double x) noexcept
    {
        return std::log2(x);
    }

    // --------------------------------------------------------------------------------------------
    float Pow(float base, float exp) noexcept
    {
        return std::powf(base, exp);
    }

    double Pow(double base, double exp) noexcept
    {
        return std::pow(base, exp);
    }

    // --------------------------------------------------------------------------------------------
    float Fmod(float x, float y) noexcept
    {
        return std::fmodf(x, y);
    }

    double Fmod(double x, double y) noexcept
    {
        return std::fmod(x, y);
    }
}
