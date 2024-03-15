// Copyright Chad Engler

#include "math.h"

#include "float.h"
#include "ldshape.h"
#include "stdint.h"

#include "wasm/libc.wasm.h"

extern "C"
{
    // --------------------------------------------------------------------------------------------
    int __fpclassify(double x)
    {
        union { double f; uint64_t i; } u = { x };
        int e = (u.i >> 52) & 0x7ff;

        if (!e)
            return (u.i << 1) ? FP_SUBNORMAL : FP_ZERO;

        if (e == 0x7ff)
            return (u.i << 12) ? FP_NAN : FP_INFINITE;

        return FP_NORMAL;
    }

    int __fpclassifyf(float x)
    {
        union { float f; uint32_t i; } u = { x };
        int e = (u.i >> 23) & 0xff;

        if (!e)
            return (u.i << 1) ? FP_SUBNORMAL : FP_ZERO;

        if (e == 0xff)
            return (u.i << 9) ? FP_NAN : FP_INFINITE;

        return FP_NORMAL;
    }

#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
    int __fpclassifyl(long double x)
    {
        return __fpclassify(x);
    }
#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384
    int __fpclassifyl(long double x)
    {
        union ldshape u = { x };
        int e = u.i.se & 0x7fff;
        int msb = u.i.m >> 63;

        if (!e && !msb)
            return u.i.m ? FP_SUBNORMAL : FP_ZERO;

        if (e == 0x7fff)
        {
            /* The x86 variant of 80-bit extended precision only admits
            * one representation of each infinity, with the mantissa msb
            * necessarily set. The version with it clear is invalid/nan.
            * The m68k variant, however, allows either, and tooling uses
            * the version with it clear. */
            if (__BYTE_ORDER == __LITTLE_ENDIAN && !msb)
                return FP_NAN;

            return u.i.m << 1 ? FP_NAN : FP_INFINITE;
        }

        if (!msb)
            return FP_NAN;

        return FP_NORMAL;
    }
#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384
    int __fpclassifyl(long double x)
    {
        union ldshape u = { x };
        int e = u.i.se & 0x7fff;
        u.i.se = 0;

        if (!e)
            return u.i2.lo | u.i2.hi ? FP_SUBNORMAL : FP_ZERO;

        if (e == 0x7fff)
            return u.i2.lo | u.i2.hi ? FP_NAN : FP_INFINITE;

        return FP_NORMAL;
    }
#endif

    // --------------------------------------------------------------------------------------------
    int __signbitf(float x)
    {
        union { float f; uint32_t i; } y = { x };
        return y.i >> 31;
    }

    int __signbit(double x)
    {
        union { double d; uint64_t i; } y = { x };
        return y.i >> 63;
    }

#if (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
    int __signbitl(long double x)
    {
        union ldshape u = { x };
        return u.i.se >> 15;
    }
#elif LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
    int __signbitl(long double x)
    {
        return __signbit(x);
    }
#endif

    // --------------------------------------------------------------------------------------------
    double acos(double x)
    {
        return heWASM_Acos(x);
    }

    float acosf(float x)
    {
        return static_cast<float>(heWASM_Acos(x));
    }

    // --------------------------------------------------------------------------------------------
    double asin(double x)
    {
        return heWASM_Asin(x);
    }

    float asinf(float x)
    {
        return static_cast<float>(heWASM_Asin(x));
    }

    // --------------------------------------------------------------------------------------------
    double atan(double x)
    {
        return heWASM_Atan(x);
    }

    float atanf(float x)
    {
        return static_cast<float>(heWASM_Atan(x));
    }

    // --------------------------------------------------------------------------------------------
    double atan2(double y, double x)
    {
        return heWASM_Atan2(y, x);
    }

    float atan2f(float y, float x)
    {
        return static_cast<float>(heWASM_Atan2(y, x));
    }

    // --------------------------------------------------------------------------------------------
    double ceil(double x)
    {
        // `f64.ceil` instruction on WASM.
        return __builtin_ceil(x);
    }

    float ceilf(float x)
    {
        // `f32.ceil` instruction on WASM.
        return __builtin_ceilf(x);
    }

    // --------------------------------------------------------------------------------------------
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

    // --------------------------------------------------------------------------------------------
    double cos(double x)
    {
        return heWASM_Cos(x);
    }

    float cosf(float x)
    {
        return static_cast<float>(heWASM_Cos(x));
    }

    // --------------------------------------------------------------------------------------------
    double exp(double x)
    {
        return heWASM_Exp(x);
    }

    float expf(float x)
    {
        return static_cast<float>(heWASM_Exp(x));
    }

    // --------------------------------------------------------------------------------------------
    double fabs(double x)
    {
        // `f64.abs` instruction on WASM.
        return __builtin_fabs(x);
    }

    float fabsf(float x)
    {
        // `f32.abs` instruction on WASM.
        return __builtin_fabsf(x);
    }

    // --------------------------------------------------------------------------------------------
    double floor(double x)
    {
        // `f64.floor` instruction on WASM.
        return __builtin_floor(x);
    }

    float floorf(float x)
    {
        // `f32.floor` instruction on WASM.
        return __builtin_floorf(x);
    }

    // --------------------------------------------------------------------------------------------
    double fmax(double x, double y)
    {
        // `f64.max` instruction on WASM.
        return __builtin_wasm_max_f64(x, y);
    }

    float fmaxf(float x, float y)
    {
        // `f32.max` instruction on WASM.
        return __builtin_wasm_max_f32(x, y);
    }

    // --------------------------------------------------------------------------------------------
    double fmin(double x, double y)
    {
        // `f64.min` instruction on WASM.
        return __builtin_wasm_min_f64(x, y);
    }

    float fminf(float x, float y)
    {
        // `f32.min` instruction on WASM.
        return __builtin_wasm_min_f32(x, y);
    }

    // --------------------------------------------------------------------------------------------
    double fmod(double x, double y)
    {
        return heWASM_Mod(x, y);
    }

    float fmodf(float x, float y)
    {
        return static_cast<float>(heWASM_Mod(x, y));
    }

    // --------------------------------------------------------------------------------------------
    double log(double x)
    {
        return heWASM_Log(x);
    }

    float logf(float x)
    {
        return static_cast<float>(heWASM_Log(x));
    }

    // --------------------------------------------------------------------------------------------
    double log10(double x)
    {
        return heWASM_Log10(x);
    }

    float log10f(float x)
    {
        return static_cast<float>(heWASM_Log10(x));
    }

    // --------------------------------------------------------------------------------------------
    double log1p(double x)
    {
        return heWASM_Log1p(x);
    }

    float log1pf(float x)
    {
        return static_cast<float>(heWASM_Log1p(x));
    }

    // --------------------------------------------------------------------------------------------
    double log2(double x)
    {
        return heWASM_Log2(x);
    }

    float log2f(float x)
    {
        return static_cast<float>(heWASM_Log2(x));
    }

    // --------------------------------------------------------------------------------------------
    double pow(double x, double y)
    {
        return heWASM_Pow(x, y);
    }

    float powf(float x, float y)
    {
        return static_cast<float>(heWASM_Pow(x, y));
    }

    // --------------------------------------------------------------------------------------------
    double round(double x)
    {
        // `f64.nearest` instruction on WASM.
        // All WASM operations use round-to-nearest ties-to-even. This is different behavior than
        // the C standard library, which uses round-to-nearest ties-away-from-zero.
        // return __builtin_roundeven(x);
        return heWASM_Round(x);
    }

    float roundf(float x)
    {
        // `f32.nearest` instruction on WASM.
        // All WASM operations use round-to-nearest ties-to-even. This is different behavior than
        // the C standard library, which uses round-to-nearest ties-away-from-zero.
        // return __builtin_roundevenf(x);
        return static_cast<float>(heWASM_Round(x));
    }

    // --------------------------------------------------------------------------------------------
    double sin(double x)
    {
        return heWASM_Sin(x);
    }

    float sinf(float x)
    {
        return static_cast<float>(heWASM_Sin(x));
    }

    // --------------------------------------------------------------------------------------------
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

    // --------------------------------------------------------------------------------------------
    double sqrt(double x)
    {
        // `f64.sqrt` instruction on WASM.
        return __builtin_sqrt(x);
    }

    float sqrtf(float x)
    {
        // `f32.sqrt` instruction on WASM.
        return __builtin_sqrtf(x);
    }

    // --------------------------------------------------------------------------------------------
    double tan(double x)
    {
        return heWASM_Tan(x);
    }

    float tanf(float x)
    {
        return static_cast<float>(heWASM_Tan(x));
    }

    // --------------------------------------------------------------------------------------------
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
