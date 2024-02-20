// Copyright Chad Engler

#pragma once

#include "alltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void* __restrict dst, const void* __restrict src, size_t len) { __builtin_memcpy(dst, src, len); }
void* memmove(void* dst, const void* src, size_t len) { return __builtin_memmove(dst, src, len); }
void* memset(void* mem, int ch, size_t len) { return __builtin_memset(mem, ch, len); }

inline int memcmp(const void* a, const void* b, size_t len)
{
    const unsigned char* a8 = static_cast<const unsigned char*>(a);
    const unsigned char* b8 = static_cast<const unsigned char*>(b);

    while (len)
    {
        if (*a8 != *b8)
            return *a8 - *b8;

        ++a8;
        ++b8;
        --len;
    }

    return 0;
}

inline const void* memchr(const void* mem, int ch, size_t len)
{
    const unsigned char* mem8 = static_cast<const unsigned char*>(mem);
    ch = static_cast<unsigned char>(ch);

    while (len)
    {
        if (*mem8 == ch)
            return mem8;

        ++mem8;
        --len;
    }

    return nullptr;
}

#ifdef __cplusplus
}
#endif
