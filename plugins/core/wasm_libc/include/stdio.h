// Copyright Chad Engler

#pragma once

#include "alltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef EOF
#define EOF (-1)

int remove(const char* path); // Not implemented, just so the compiler can resolve the symbol.

#ifdef __cplusplus
}
#endif
