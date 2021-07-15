// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/types.h"

#if HE_COMPILER_MSVC
    extern "C"
    {
        void* __cdecl memcpy(void*, const void*, size_t);
        int __cdecl memcmp(const void*, const void*, size_t);
        void* __cdecl memset(void*, int, size_t);
        _VCRTIMP void* __cdecl memmove(void*, const void*, size_t);
        _VCRTIMP const void* __cdecl memchr(const void*, int, size_t);
    }
#endif

namespace he
{
#if HE_COMPILER_GCC || HE_COMPILER_CLANG
    HE_FORCE_INLINE void* MemCopy(void* dst, const void* src, size_t len) { return __builtin_memcpy(dst, src, len); }
    HE_FORCE_INLINE void* MemMove(void* dst, const void* src, size_t len) { return __builtin_memmove(dst, src, len); }
    HE_FORCE_INLINE int32_t MemCmp(const void* a, const void* b, size_t len) { return __builtin_memcmp(a, b, len); }
    HE_FORCE_INLINE void* MemSet(void* mem, int ch, size_t len) { return __builtin_memset(mem, ch, len); }
    HE_FORCE_INLINE const void* MemChr(const void* mem, int ch, size_t len) { return __builtin_memchr(mem, ch, len); }
#elif HE_COMPILER_MSVC
    HE_FORCE_INLINE void* MemCopy(void* dst, const void* src, size_t len) { return memcpy(dst, src, len); }
    HE_FORCE_INLINE void* MemMove(void* dst, const void* src, size_t len) { return memmove(dst, src, len); }
    HE_FORCE_INLINE int32_t MemCmp(const void* a, const void* b, size_t len) { return memcmp(a, b, len); }
    HE_FORCE_INLINE void* MemSet(void* mem, int ch, size_t len) { return memset(mem, ch, len); }
    HE_FORCE_INLINE const void* MemChr(const void* mem, int ch, size_t len) { return memchr(mem, ch, len); }
#endif

    HE_FORCE_INLINE void* MemZero(void* dst, size_t count) { return MemSet(dst, 0, count); }
    HE_FORCE_INLINE bool MemEqual(const void* a, const void* b, size_t count) { return MemCmp(a, b, count) == 0; }
    HE_FORCE_INLINE bool MemLess(const void* a, const void* b, size_t count) { return MemCmp(a, b, count) < 0; }
}
