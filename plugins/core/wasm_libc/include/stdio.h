// Copyright Chad Engler

#pragma once

#include "alltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef EOF
#define EOF (-1)

int remove(const char* path); // Not implemented, just so the compiler can resolve the symbol.

int sprintf(char* __restrict s, const char* __restrict fmt, ...);
int snprintf(char* __restrict s, size_t n, const char* __restrict fmt, ...);

int vsprintf(char* __restrict s, const char* __restrict fmt, __builtin_va_list args);
int vsnprintf(char* __restrict s, size_t n, const char* __restrict fmt, __builtin_va_list args);

#ifdef __cplusplus
}
#endif
