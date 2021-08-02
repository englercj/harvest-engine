// Copyright Chad Engler

namespace he
{
    constexpr bool IsNan(float x)
    {
        return (BitCast<uint32_t>(x) & INT32_MAX) > 0x7f800000u;
    }

    constexpr bool IsInfinite(float x)
    {
        return (BitCast<uint32_t>(x) & INT32_MAX) == 0x7f800000u;
    }

    constexpr bool IsFinite(float x)
    {
        return (BitCast<uint32_t>(x) & INT32_MAX) < 0x7f800000u;
    }

    constexpr bool IsZeroSafe(float x)
    {
        return Abs(x) > Float_ZeroSafe;
    }

    constexpr float Floor(float x)
    {
        HE_ASSERT(!IsNan(x) && IsFinite(x));
        const float f = static_cast<float>(static_cast<int32_t>(x));
        return x < f ? f - 1 : f;
    }

    constexpr float Ceil(float x)
    {
        HE_ASSERT(!IsNan(x) && IsFinite(x));
        const float f = static_cast<float>(static_cast<int32_t>(x));
        return x > f ? f + 1 : f;
    }

    constexpr float ToRadians(float deg)
    {
        return (deg * Float_Pi) / 180.0f;
    }

    constexpr float ToDegrees(float rad)
    {
        return (rad * 180.0f) / Float_Pi;
    }

    constexpr float Lerp(float a, float b, float t)
    {
        return a + ((b - a) * t);
    }

    constexpr float SmoothStep(float a, float b, float t)
    {
        float c = Clamp((t - a) / (b - a), 0.0f, 1.0f);
        return c * c * (3.0f - (2.0f * c));
    }

    constexpr float Rcp(float x)
    {
        HE_ASSERT(IsZeroSafe(x));
        return 1.0f / x;
    }

    constexpr float RcpSafe(float x)
    {
        return IsZeroSafe(x) ? (1.0f / x) : 0.0f;
    }

    // TODO: Do SSE/NEON implementations of some of these trig functions.
    // For example: http://gruntthepeon.free.fr/ssemath/

    inline float Sqrt(float x)
    {
        HE_ASSERT(!IsNan(x) && x >= -0.0f);
        return sqrtf(x);
    }

    inline float Rsqrt(float x)
    {
        return 1.0f / Sqrt(x);
    }

    inline float Sin(float x)
    {
        HE_ASSERT(!IsNan(x) && IsFinite(x));
        return sinf(x);
    }

    inline float Asin(float x)
    {
        HE_ASSERT(!IsNan(x) && x >= -1.0f && x <= 1.0f);
        return asinf(x);
    }

    inline float Cos(float x)
    {
        HE_ASSERT(!IsNan(x) && IsFinite(x));
        return cosf(x);
    }

    inline float Acos(float x)
    {
        HE_ASSERT(!IsNan(x) && x >= -1.0f && x <= 1.0f);
        return acosf(x);
    }

    inline float Tan(float x)
    {
        HE_ASSERT(!IsNan(x) && IsFinite(x));
        return tanf(x);
    }

    inline float Atan(float x)
    {
        HE_ASSERT(!IsNan(x));
        return atanf(x);
    }

    inline float Atan2(float y, float x)
    {
        HE_ASSERT(!IsNan(y) && !IsNan(x));
        return atan2f(y, x);
    }

    inline void SinCos(float x, float& s, float& c)
    {
        // TODO: Optimize
        s = Sin(x);
        c = Cos(x);
    }

    inline float Exp(float x)
    {
        HE_ASSERT(!IsNan(x));
        return expf(x);
    }

    inline float Ln(float x)
    {
        HE_ASSERT(!IsNan(x) && x > 0);
        return logf(x);
    }

    inline float Lb(float x)
    {
        HE_ASSERT(!IsNan(x) && x > 0);
        return log2f(x);
    }

    inline float LogN(float x, float n)
    {
        return Ln(x) / Ln(n);
    }

    inline float Pow(float base, float exp)
    {
        HE_ASSERT(!IsNan(base) && !IsNan(exp));
        HE_ASSERT(!(IsFinite(base) && base < 0 && IsFinite(exp) && floorf(exp) != exp)); // powf returns NaN
        return powf(base, exp);
    }

    inline float Fmod(float x, float y)
    {
        HE_ASSERT(!IsNan(x) && !IsNan(y) && IsFinite(x));
        HE_ASSERT(y != 0.0f && y != -0.0f);
        return fmodf(x, y);
    }
}
