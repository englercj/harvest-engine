// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/types.h"

#if HE_COMPILER_MSVC
    extern "C"
    {
        void* __cdecl memcpy(void*, const void*, size_t);
        int __cdecl memcmp(const void*, const void*, size_t);
        void* __cdecl memset(void*, int, size_t);
        void* __cdecl memmove(void*, const void*, size_t);
        const void* __cdecl memchr(const void*, int, size_t);

        unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64);
        unsigned long __cdecl _byteswap_ulong(unsigned long);
        unsigned short __cdecl _byteswap_ushort(unsigned short);
    }
#pragma intrinsic(memcpy)
#pragma intrinsic(memcmp)
#pragma intrinsic(memset)
#endif

namespace he
{
#if HE_COMPILER_GCC || HE_COMPILER_CLANG
    HE_FORCE_INLINE void* MemCopy(void* dst, const void* src, size_t len) { return __builtin_memcpy(dst, src, len); }
    HE_FORCE_INLINE void* MemMove(void* dst, const void* src, size_t len) { return __builtin_memmove(dst, src, len); }
    HE_FORCE_INLINE int32_t MemCmp(const void* a, const void* b, size_t len) { return __builtin_memcmp(a, b, len); }
    HE_FORCE_INLINE void* MemSet(void* mem, int ch, size_t len) { return __builtin_memset(mem, ch, len); }
    HE_FORCE_INLINE const void* MemChr(const void* mem, int ch, size_t len) { return __builtin_memchr(mem, ch, len); }
    HE_FORCE_INLINE uint16_t ByteSwap(uint16_t x) { return (x >> 8) | (x << 8); }
    HE_FORCE_INLINE uint32_t ByteSwap(uint32_t x) { return __builtin_bswap32(x); }
    HE_FORCE_INLINE uint64_t ByteSwap(uint64_t x) { return __builtin_bswap64(x); }
#elif HE_COMPILER_MSVC
    HE_FORCE_INLINE void* MemCopy(void* dst, const void* src, size_t len) { return memcpy(dst, src, len); }
    HE_FORCE_INLINE void* MemMove(void* dst, const void* src, size_t len) { return memmove(dst, src, len); }
    HE_FORCE_INLINE int32_t MemCmp(const void* a, const void* b, size_t len) { return memcmp(a, b, len); }
    HE_FORCE_INLINE void* MemSet(void* mem, int ch, size_t len) { return memset(mem, ch, len); }
    HE_FORCE_INLINE const void* MemChr(const void* mem, int ch, size_t len) { return memchr(mem, ch, len); }
    HE_FORCE_INLINE uint16_t ByteSwap(uint16_t x) { return _byteswap_ushort(x); }
    HE_FORCE_INLINE uint32_t ByteSwap(uint32_t x) { return _byteswap_ulong(x); }
    HE_FORCE_INLINE uint64_t ByteSwap(uint64_t x) { return _byteswap_uint64(x); }
#endif

    HE_FORCE_INLINE void* MemZero(void* dst, size_t count) { return MemSet(dst, 0, count); }
    HE_FORCE_INLINE bool MemEqual(const void* a, const void* b, size_t count) { return MemCmp(a, b, count) == 0; }
    HE_FORCE_INLINE bool MemLess(const void* a, const void* b, size_t count) { return MemCmp(a, b, count) < 0; }

#if HE_CPU_LITTLE_ENDIAN
    static inline uint16_t LoadLE(const uint16_t& p) { return p; }
    static inline uint32_t LoadLE(const uint32_t& p) { return p; }
    static inline uint64_t LoadLE(const uint64_t& p) { return p; }

    static inline uint16_t LoadBE(const uint16_t& p) { return ByteSwap(p); }
    static inline uint32_t LoadBE(const uint32_t& p) { return ByteSwap(p); }
    static inline uint64_t LoadBE(const uint64_t& p) { return ByteSwap(p); }

    static inline void StoreLE(uint16_t& p, uint16_t x) { p = x; }
    static inline void StoreLE(uint32_t& p, uint32_t x) { p = x; }
    static inline void StoreLE(uint64_t& p, uint64_t x) { p = x; }

    static inline void StoreBE(uint16_t& p, uint16_t x) { p = ByteSwap(x); }
    static inline void StoreBE(uint32_t& p, uint32_t x) { p = ByteSwap(x); }
    static inline void StoreBE(uint64_t& p, uint64_t x) { p = ByteSwap(x); }
#else
    static inline uint16_t LoadLE(const uint16_t& p) { return ByteSwap(p); }
    static inline uint32_t LoadLE(const uint32_t& p) { return ByteSwap(p); }
    static inline uint64_t LoadLE(const uint64_t& p) { return ByteSwap(p); }

    static inline uint16_t LoadBE(const uint16_t& p) { return p; }
    static inline uint32_t LoadBE(const uint32_t& p) { return p; }
    static inline uint64_t LoadBE(const uint64_t& p) { return p; }

    static inline void StoreLE(uint16_t& p, uint16_t x) { p = ByteSwap(x); }
    static inline void StoreLE(uint32_t& p, uint32_t x) { p = ByteSwap(x); }
    static inline void StoreLE(uint64_t& p, uint64_t x) { p = ByteSwap(x); }

    static inline void StoreBE(uint16_t& p, uint16_t x) { p = x; }
    static inline void StoreBE(uint32_t& p, uint32_t x) { p = x; }
    static inline void StoreBE(uint64_t& p, uint64_t x) { p = x; }
#endif
}
