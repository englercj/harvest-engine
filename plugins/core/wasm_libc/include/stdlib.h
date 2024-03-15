// Copyright Chad Engler

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

float strtof(const char* __restrict s, char** __restrict end);
double strtod(const char* __restrict s, char** __restrict end);
long double strtold(const char* __restrict s, char** __restrict end);

long strtol(const char* __restrict s, char** __restrict end, int base);
unsigned long strtoul(const char* __restrict s, char** __restrict end, int base);
long long strtoll(const char* __restrict s, char** __restrict end, int base);
unsigned long long strtoull(const char* __restrict s, char** __restrict end, int base);

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);
void* aligned_alloc(size_t alignment, size_t size);

__attribute__((__noreturn__)) void abort();

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

inline div_t div(int num, int den) { return (div_t){ num/den, num%den }; }
inline ldiv_t ldiv(long num, long den) { return (ldiv_t){ num/den, num%den }; }
inline lldiv_t lldiv(long long num, long long den) { return (lldiv_t){ num/den, num%den }; }

#ifdef __cplusplus
}
#endif
