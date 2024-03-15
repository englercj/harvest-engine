// Copyright Chad Engler

#pragma once

#include "alltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

inline void* memcpy(void* __restrict dst, const void* __restrict src, size_t len) { __builtin_memcpy(dst, src, len); }
inline void* memmove(void* dst, const void* src, size_t len) { return __builtin_memmove(dst, src, len); }
inline void* memset(void* mem, int ch, size_t len) { return __builtin_memset(mem, ch, len); }
inline int memcmp(const void* a, const void* b, size_t len) { return __builtin_memcmp(a, b, len); }
inline void* memchr(const void* mem, int ch, size_t len) { return __builtin_memchr(mem, ch, len); }

inline char* strcpy(char* __restrict dst, const char* __restrict src) { return __builtin_strcpy(dst, src); }
inline char* strncpy(char* __restrict dst, const char* __restrict src, size_t len) { return __builtin_strncpy(dst, src, len); }

inline char* strcat(char* __restrict dst, const char* __restrict src) { return __builtin_strcat(dst, src); }
inline char* strncat(char* __restrict dst, const char* __restrict src, size_t len) { return __builtin_strncat(dst, src, len); }

inline int strcmp(const char* a, const char* b) { return __builtin_strcmp(a, b); }
inline int strncmp(const char* a, const char* b, size_t len) { return __builtin_strncmp(a, b, len); }

inline char* strchr(const char* str, int ch) { return __builtin_strchr(str, ch); }
inline char* strrchr(const char* str, int ch) { return __builtin_strrchr(str, ch); }

inline size_t strlen(const char* str) { return __builtin_strlen(str); }
inline size_t strnlen(const char* str, size_t len)
{
    const char* p = static_cast<const char*>(memchr(str, 0, len));
    return p ? (p - str) : len;
}

inline char* strdup(const char* src) { return __builtin_strdup(src); }
inline char* strndup(const char* src, size_t len) { return __builtin_strndup(src, len); }

char* strerror(int e);
int strerror_r(int e, char* dst, size_t dstLen);

#ifdef __cplusplus
}
#endif
