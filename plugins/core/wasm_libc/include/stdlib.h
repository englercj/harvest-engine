// Copyright Chad Engler

#pragma once

#include "_alltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

int atoi(const char* s);
long atol(const char* s);
long long atoll(const char* s);
double atof(const char* s);

float strtof(const char* __restrict s, char** __restrict end);
double strtod(const char* __restrict s, char** __restrict end);
long double strtold(const char* __restrict s, char** __restrict end);

long strtol(const char* __restrict s, char** __restrict end, int base);
unsigned long strtoul(const char* __restrict s, char** __restrict end, int base);
long long strtoll(const char* __restrict s, char** __restrict end, int base);
unsigned long long strtoull(const char* __restrict s, char** __restrict end, int base);

#define RAND_MAX (0x7fffffff)
int rand();
void srand(unsigned seed);

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);
void* aligned_alloc(size_t alignment, size_t size);

__attribute__((__noreturn__)) void abort();
int atexit(void(*proc)(void));
__attribute__((__noreturn__)) void exit(int rc);
__attribute__((__noreturn__)) void _Exit(int rc);
int at_quick_exit(void(*proc)(void));
__attribute__((__noreturn__)) void quick_exit(int rc);

char* getenv(const char* name);

int abs(int x);
long labs(long x);
long long llabs(long long x);

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

inline div_t div(int num, int den) { return (div_t){ num/den, num%den }; }
inline ldiv_t ldiv(long num, long den) { return (ldiv_t){ num/den, num%den }; }
inline lldiv_t lldiv(long long num, long long den) { return (lldiv_t){ num/den, num%den }; }

#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
    || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) \
    || defined(_BSD_SOURCE)

int setenv(const char* name, const char* value, int overwrite);
int unsetenv(const char* name);

#endif

#ifdef __cplusplus
}
#endif
