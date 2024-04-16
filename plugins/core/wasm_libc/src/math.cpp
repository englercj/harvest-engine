// Copyright Chad Engler

#include "math.h"

#include "float.h"
#include "ldshape.h"
#include "stdint.h"

#include "he/core/math.h"

extern "C"
{
    // --------------------------------------------------------------------------------------------
    static int _FpClassToInt(he::FpClass fpClass)
    {
        switch (fpClass)
        {
            case he::FpClass::Normal: return FP_NORMAL;
            case he::FpClass::Subnormal: return FP_SUBNORMAL;
            case he::FpClass::Zero: return FP_ZERO;
            case he::FpClass::Infinite: return FP_INFINITE;
            case he::FpClass::Nan: return FP_NAN;
        }
    }

    int __fpclassifyf(float x) { return _FpClassToInt(he::Classify(x)); }
    int __fpclassify(double x) { return _FpClassToInt(he::Classify(x)); }
    int __fpclassifyl(long double x) { return _FpClassToInt(he::Classify(x)); }

    int __signbitf(float x) { return static_cast<int>(he::HasSignBit(x)); }
    int __signbit(double x) { return static_cast<int>(he::HasSignBit(x)); }
    int __signbitl(long double x) { return static_cast<int>(he::HasSignBit(x)); }

    // --------------------------------------------------------------------------------------------
    double acos(double x) { return he::Acos(x); }
    float acosf(float x) { return he::Acos(x); }

    double asin(double x) { return he::Asin(x); }
    float asinf(float x) { return he::Asin(x); }

    double atan(double x) { return he::Atan(x); }
    float atanf(float x) { return he::Atan(x); }

    double atan2(double y, double x) { return he::Atan2(y, x); }
    float atan2f(float y, float x) { return he::Atan2(y, x); }

    double ceil(double x) { return he::Ceil(x); }
    float ceilf(float x) { return he::Ceil(x); }

    double copysign(double x, double y)
    {
        // `f64.copysign` instruction on WASM.
        return __builtin_copysign(x, y);
    }

    float copysignf(float x, float y)
    {
        // `f32.copysign` instruction on WASM.
        return __builtin_copysignf(x, y);
    }

    double cos(double x) { return he::Cos(x); }
    float cosf(float x) { return he::Cos(x); }

    double exp(double x) { return he::Exp(x); }
    float expf(float x) { return he::Exp(x); }

    double fabs(double x) { return he::Abs(x); }
    float fabsf(float x) { return he::Abs(x); }

    double floor(double x) { return he::Floor(x); }
    float floorf(float x) { return he::Floor(x); }

    double fmax(double x, double y) { return he::Max(x, y); }
    float fmaxf(float x, float y) { return he::Max(x, y); }

    double fmin(double x, double y) { return he::Min(x, y); }
    float fminf(float x, float y) { return he::Min(x, y); }

    double fmod(double x, double y) { return he::Fmod(x, y); }
    float fmodf(float x, float y) { return he::Fmod(x, y); }

    double log(double x) { return he::Log(x); }
    float logf(float x) { return he::Log(x); }

    double log10(double x) { return he::Log10(x); }
    float log10f(float x) { return he::Log10(x); }

    double log1p(double x) { return he::Log1p(x); }
    float log1pf(float x) { return he::Log1p(x); }

    double log2(double x) { return he::Log2(x); }
    float log2f(float x) { return he::Log2(x); }

    double pow(double x, double y) { return he::Pow(x, y); }
    float powf(float x, float y) { return he::Pow(x, y); }

    double round(double x) { return he::Round(x); }
    float roundf(float x) { return he::Round(x); }

    double sin(double x) { return he::Sin(x); }
    float sinf(float x) { return he::Sin(x); }

    void sincos(double x, double* s, double* c)
    {
        *s = sin(x);
        *c = cos(x);
    }

    void sincosf(float x, float* s, float* c)
    {
        *s = sinf(x);
        *c = cosf(x);
    }

    double sqrt(double x) { return he::Sqrt(x); }
    float sqrtf(float x) { return he::Sqrt(x); }

    double tan(double x) { return he::Tan(x); }
    float tanf(float x) { return he::Tan(x); }

    double trunc(double x)
    {
        // `f64.trunc` instruction on WASM.
        return __builtin_trunc(x);
    }

    float truncf(float x)
    {
        // `f32.trunc` instruction on WASM.
        return __builtin_truncf(x);
    }
}
