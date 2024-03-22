// Copyright Chad Engler

#pragma once

#include "_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NAN             __builtin_nanf("")
#define INFINITY        __builtin_inff()

#define HUGE_VALF       INFINITY
#define HUGE_VAL        ((double)INFINITY)
#define HUGE_VALL       ((long double)INFINITY)

#define FP_ILOGBNAN     (-1-0x7fffffff)
#define FP_ILOGB0       FP_ILOGBNAN

#define FP_NAN          0
#define FP_INFINITE     1
#define FP_ZERO         2
#define FP_SUBNORMAL    3
#define FP_NORMAL       4

#ifdef __FP_FAST_FMA
    #define FP_FAST_FMA 1
#endif

#ifdef __FP_FAST_FMAF
    #define FP_FAST_FMAF 1
#endif

#ifdef __FP_FAST_FMAL
    #define FP_FAST_FMAL 1
#endif

int __fpclassifyf(float);
int __fpclassify(double);
int __fpclassifyl(long double);

int __signbitf(float);
int __signbit(double);
int __signbitl(long double);

_Forceinline unsigned __FLOAT_BITS(float __f)
{
    union { float __f; unsigned __i; } __u;
    __u.__f = __f;
    return __u.__i;
}
_Forceinline unsigned long long __DOUBLE_BITS(double __f)
{
    union { double __f; unsigned long long __i; } __u;
    __u.__f = __f;
    return __u.__i;
}

#define fpclassify(x) ( \
    sizeof(x) == sizeof(float) ? __fpclassifyf(x) \
    : sizeof(x) == sizeof(double) ? __fpclassify(x) \
    : __fpclassifyl(x))

#define isinf(x) ( \
    sizeof(x) == sizeof(float) ? ((__FLOAT_BITS(x) & 0x7fffffff) == 0x7f800000) \
    : sizeof(x) == sizeof(double) ? ((__DOUBLE_BITS(x) & (-1ULL >> 1)) == (0x7ffULL << 52)) \
    : __fpclassifyl(x) == FP_INFINITE)

#define isnan(x) ( \
    sizeof(x) == sizeof(float) ? ((__FLOAT_BITS(x) & 0x7fffffff) > 0x7f800000) \
    : sizeof(x) == sizeof(double) ? ((__DOUBLE_BITS(x) & (-1ULL >> 1)) > (0x7ffULL << 52)) \
    : __fpclassifyl(x) == FP_NAN)

#define isnormal(x) ( \
    sizeof(x) == sizeof(float) ? (((__FLOAT_BITS(x) + 0x00800000) & 0x7fffffff) >= 0x01000000) \
    : sizeof(x) == sizeof(double) ? (((__DOUBLE_BITS(x) + (1ULL << 52)) & (-1ULL >> 1)) >= (1ULL << 53)) \
    : __fpclassifyl(x) == FP_NORMAL)

#define isfinite(x) ( \
    sizeof(x) == sizeof(float) ? ((__FLOAT_BITS(x) & 0x7fffffff) < 0x7f800000) \
    : sizeof(x) == sizeof(double) ? ((__DOUBLE_BITS(x) & (-1ULL >> 1)) < (0x7ffULL << 52)) \
    : __fpclassifyl(x) > FP_INFINITE)

#define signbit(x) ( \
    sizeof(x) == sizeof(float) ? ((int)(__FLOAT_BITS(x)>>31)) \
    : sizeof(x) == sizeof(double) ? ((int)(__DOUBLE_BITS(x)>>63)) \
    : __signbitl(x))

#define isunordered(x, y) (isnan((x)) ? ((void)(y),1) : isnan((y)))

double      acos(double x);
float       acosf(float x);

double      asin(double x);
float       asinf(float x);

double      atan(double x);
float       atanf(float x);

double      atan2(double y, double x);
float       atan2f(float y, float x);

double      ceil(double x);
float       ceilf(float x);

double      copysign(double x, double y);
float       copysignf(float x, float y);

double      cos(double x);
float       cosf(float x);

double      exp(double x);
float       expf(float x);

double      fabs(double x);
float       fabsf(float x);

double      floor(double x);
float       floorf(float x);

double      fmax(double x, double y);
float       fmaxf(float x, float y);

double      fmin(double x, double y);
float       fminf(float x, float y);

double      fmod(double x, double y);
float       fmodf(float x, float y);

double      log(double x);
float       logf(float x);

double      log10(double x);
float       log10f(float x);

double      log1p(double x);
float       log1pf(float x);

double      log2(double x);
float       log2f(float x);

double      pow(double x, double y);
float       powf(float x, float y);

double      round(double x);
float       roundf(float x);

double      sin(double x);
float       sinf(float x);

void        sincos(double x, double* s, double* c);
void        sincosf(float x, float* s, float* c);

double      sqrt(double x);
float       sqrtf(float x);

double      tan(double x);
float       tanf(float x);

double      trunc(double x);
float       truncf(float x);

#define MAXFLOAT        3.40282346638528859812e+38F

#define M_E             2.7182818284590452354   /* e */
#define M_LOG2E         1.4426950408889634074   /* log_2 e */
#define M_LOG10E        0.43429448190325182765  /* log_10 e */
#define M_LN2           0.69314718055994530942  /* log_e 2 */
#define M_LN10          2.30258509299404568402  /* log_e 10 */
#define M_PI            3.14159265358979323846  /* pi */
#define M_PI_2          1.57079632679489661923  /* pi/2 */
#define M_PI_4          0.78539816339744830962  /* pi/4 */
#define M_1_PI          0.31830988618379067154  /* 1/pi */
#define M_2_PI          0.63661977236758134308  /* 2/pi */
#define M_2_SQRTPI      1.12837916709551257390  /* 2/sqrt(pi) */
#define M_SQRT2         1.41421356237309504880  /* sqrt(2) */
#define M_SQRT1_2       0.70710678118654752440  /* 1/sqrt(2) */

#ifdef __cplusplus
}
#endif
