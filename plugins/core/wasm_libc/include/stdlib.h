// Copyright Chad Engler

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__)) void abort(void) {}

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

inline div_t div(int num, int den)
{
    return (div_t){ num/den, num%den };
}

inline ldiv_t ldiv(long num, long den)
{
    return (ldiv_t){ num/den, num%den };
}

inline lldiv_t lldiv(long long num, long long den)
{
    return (lldiv_t){ num/den, num%den };
}

#ifdef __cplusplus
}
#endif
