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

        unsigned int __cdecl _rotl(unsigned int, int);
        unsigned __int64 __cdecl _rotl64(unsigned __int64, int);
        unsigned int __cdecl _rotr(unsigned int, int);
        unsigned __int64 __cdecl _rotr64(unsigned __int64, int);
    }
    #pragma intrinsic(memcpy, memcmp, memset, memmove, memchr)
    #pragma intrinsic(_byteswap_uint64, _byteswap_ulong, _byteswap_ushort)
    #pragma intrinsic(_rotl, _rotl64, _rotr, _rotr64)
#endif

namespace he
{
#if defined(HE_PLATFORM_WASM)
    HE_FORCE_INLINE void* MemCopy(void* dst, const void* src, size_t len) { return __builtin_memcpy(dst, src, len); }
    HE_FORCE_INLINE void* MemMove(void* dst, const void* src, size_t len) { return __builtin_memmove(dst, src, len); }
    int32_t MemCmp(const void* a, const void* b, size_t len);
    HE_FORCE_INLINE void* MemSet(void* mem, int ch, size_t len) { return __builtin_memset(mem, ch, len); }
    const void* MemChr(const void* mem, int ch, size_t len);
#elif HE_COMPILER_MSVC
    HE_FORCE_INLINE void* MemCopy(void* dst, const void* src, size_t len) { return memcpy(dst, src, len); }
    HE_FORCE_INLINE void* MemMove(void* dst, const void* src, size_t len) { return memmove(dst, src, len); }
    HE_FORCE_INLINE int32_t MemCmp(const void* a, const void* b, size_t len) { return memcmp(a, b, len); }
    HE_FORCE_INLINE void* MemSet(void* mem, int ch, size_t len) { return memset(mem, ch, len); }
    HE_FORCE_INLINE const void* MemChr(const void* mem, int ch, size_t len) { return memchr(mem, ch, len); }

    HE_FORCE_INLINE uint16_t ByteSwap(uint16_t x) { return _byteswap_ushort(x); }
    HE_FORCE_INLINE uint32_t ByteSwap(uint32_t x) { return _byteswap_ulong(x); }
    HE_FORCE_INLINE uint64_t ByteSwap(uint64_t x) { return _byteswap_uint64(x); }

    HE_FORCE_INLINE uint32_t Rotl32(uint32_t x, uint32_t r) { return _rotl(x, r); }
    HE_FORCE_INLINE uint64_t Rotl64(uint64_t x, uint32_t r) { return _rotl64(x, r); }
    HE_FORCE_INLINE uint32_t Rotr32(uint32_t x, uint32_t r) { return _rotr(x, r); }
    HE_FORCE_INLINE uint64_t Rotr64(uint64_t x, uint32_t r) { return _rotr64(x, r); }
#else
    HE_FORCE_INLINE void* MemCopy(void* dst, const void* src, size_t len) { return __builtin_memcpy(dst, src, len); }
    HE_FORCE_INLINE void* MemMove(void* dst, const void* src, size_t len) { return __builtin_memmove(dst, src, len); }
    HE_FORCE_INLINE int32_t MemCmp(const void* a, const void* b, size_t len) { return __builtin_memcmp(a, b, len); }
    HE_FORCE_INLINE void* MemSet(void* mem, int ch, size_t len) { return __builtin_memset(mem, ch, len); }
    HE_FORCE_INLINE const void* MemChr(const void* mem, int ch, size_t len) { return __builtin_memchr(mem, ch, len); }

    HE_FORCE_INLINE uint16_t ByteSwap(uint16_t x) { return (x >> 8) | (x << 8); }
    HE_FORCE_INLINE uint32_t ByteSwap(uint32_t x) { return __builtin_bswap32(x); }
    HE_FORCE_INLINE uint64_t ByteSwap(uint64_t x) { return __builtin_bswap64(x); }

    HE_FORCE_INLINE uint32_t Rotl32(uint32_t x, uint32_t r) { return (x << r) | (x >> (32 - r)); }
    HE_FORCE_INLINE uint64_t Rotl64(uint64_t x, uint32_t r) { return (x << r) | (x >> (64 - r)); }
    HE_FORCE_INLINE uint32_t Rotr32(uint32_t x, uint32_t r) { return (x >> r) | (x << (32 - r)); }
    HE_FORCE_INLINE uint64_t Rotr64(uint64_t x, uint32_t r) { return (x >> r) | (x << (64 - r)); }
#endif

    HE_FORCE_INLINE void* MemZero(void* dst, size_t count) { return MemSet(dst, 0, count); }
    HE_FORCE_INLINE bool MemEqual(const void* a, const void* b, size_t count) { return MemCmp(a, b, count) == 0; }
    HE_FORCE_INLINE bool MemLess(const void* a, const void* b, size_t count) { return MemCmp(a, b, count) < 0; }

#if HE_CPU_LITTLE_ENDIAN
    HE_FORCE_INLINE uint16_t LoadLE(const uint16_t& p) { return p; }
    HE_FORCE_INLINE uint32_t LoadLE(const uint32_t& p) { return p; }
    HE_FORCE_INLINE uint64_t LoadLE(const uint64_t& p) { return p; }

    HE_FORCE_INLINE uint16_t LoadBE(const uint16_t& p) { return ByteSwap(p); }
    HE_FORCE_INLINE uint32_t LoadBE(const uint32_t& p) { return ByteSwap(p); }
    HE_FORCE_INLINE uint64_t LoadBE(const uint64_t& p) { return ByteSwap(p); }

    HE_FORCE_INLINE void StoreLE(uint16_t& p, uint16_t x) { p = x; }
    HE_FORCE_INLINE void StoreLE(uint32_t& p, uint32_t x) { p = x; }
    HE_FORCE_INLINE void StoreLE(uint64_t& p, uint64_t x) { p = x; }

    HE_FORCE_INLINE void StoreBE(uint16_t& p, uint16_t x) { p = ByteSwap(x); }
    HE_FORCE_INLINE void StoreBE(uint32_t& p, uint32_t x) { p = ByteSwap(x); }
    HE_FORCE_INLINE void StoreBE(uint64_t& p, uint64_t x) { p = ByteSwap(x); }
#else
    HE_FORCE_INLINE uint16_t LoadLE(const uint16_t& p) { return ByteSwap(p); }
    HE_FORCE_INLINE uint32_t LoadLE(const uint32_t& p) { return ByteSwap(p); }
    HE_FORCE_INLINE uint64_t LoadLE(const uint64_t& p) { return ByteSwap(p); }

    HE_FORCE_INLINE uint16_t LoadBE(const uint16_t& p) { return p; }
    HE_FORCE_INLINE uint32_t LoadBE(const uint32_t& p) { return p; }
    HE_FORCE_INLINE uint64_t LoadBE(const uint64_t& p) { return p; }

    HE_FORCE_INLINE void StoreLE(uint16_t& p, uint16_t x) { p = ByteSwap(x); }
    HE_FORCE_INLINE void StoreLE(uint32_t& p, uint32_t x) { p = ByteSwap(x); }
    HE_FORCE_INLINE void StoreLE(uint64_t& p, uint64_t x) { p = ByteSwap(x); }

    HE_FORCE_INLINE void StoreBE(uint16_t& p, uint16_t x) { p = x; }
    HE_FORCE_INLINE void StoreBE(uint32_t& p, uint32_t x) { p = x; }
    HE_FORCE_INLINE void StoreBE(uint64_t& p, uint64_t x) { p = x; }
#endif
}
