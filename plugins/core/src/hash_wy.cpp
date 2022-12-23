// Copyright Chad Engler

// This is a reduced and cleaned up implementation of wyhash by Wang Yi.
// https://github.com/wangyi-fudan/wyhash
//
// License:
//
// This is free and unencumbered software released into the public domain under The Unlicense (http://unlicense.org/)
// main repo: https://github.com/wangyi-fudan/wyhash
// author: 王一 Wang Yi <godspeed_china@yeah.net>
// contributors: Reini Urban, Dietrich Epp, Joshua Haberman, Tommy Ettinger, Daniel Lemire, Otmar Ertl, cocowalla, leo-yuriev, Diego Barrios Romero, paulie-g, dumblob, Yann Collet, ivte-ms, hyb, James Z.M. Gao, easyaspi314 (Devin), TheOneric

#include "he/core/hash.h"

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"

#if HE_COMPILER_MSVC
    #include <nmmintrin.h>

    extern "C" unsigned __int64 _umul128(unsigned __int64, unsigned __int64, unsigned __int64*);
    #pragma intrinsic(_umul128)
#endif

namespace he
{
namespace wyhash
{
    // --------------------------------------------------------------------------------------------
    static HE_FORCE_INLINE uint64_t Rot(uint64_t x)
    {
        return (x >> 32) | (x << 32);
    }

    static HE_FORCE_INLINE void Mum(uint64_t& a, uint64_t& b)
    {
    #if defined(__SIZEOF_INT128__)
        __uint128_t r = a;
        r *= b;
        a = static_cast<uint64_t>(r);
        b = static_cast<uint64_t>(r >> 64);
    #elif HE_COMPILER_MSVC && HE_CPU_X86_64
        a = _umul128(a, b, &b);
    #else
        const uint64_t ha = a >> 32;
        const uint64_t hb = b >> 32;
        const uint64_t la = static_cast<uint32_t>(a);
        const uint64_t lb = static_cast<uint32_t>(b);
        const uint64_t rh = ha * hb;
        const uint64_t rm0 = ha * lb;
        const uint64_t rm1 = hb * la;
        const uint64_t rl = la * lb;
        const uint64_t t = rl + (rm0 << 32);
        const uint64_t lo = t + (rm1 << 32);
        const uint64_t c = static_cast<uint64_t>(t < rl) + static_cast<uint64_t>(lo < t);
        const uint64_t hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
        a = lo;
        b = hi;
    #endif
    }

    [[nodiscard]] static HE_FORCE_INLINE uint64_t Mix(uint64_t a, uint64_t b)
    {
        Mum(a, b);
        return a ^ b;
    }

    [[nodiscard]] static HE_FORCE_INLINE uint64_t R8(const uint8_t* p)
    {
        uint64_t v;
        MemCopy(&v, p, 8);
        return v;
    }

    [[nodiscard]] static HE_FORCE_INLINE uint64_t R4(const uint8_t* p)
    {
        uint32_t v;
        MemCopy(&v, p, 4);
        return v;
    }

    [[nodiscard]] static HE_FORCE_INLINE uint64_t R3(const uint8_t* p, size_t k)
    {
        return (static_cast<uint64_t>(p[0]) << 16) | (static_cast<uint64_t>(p[k >> 1]) << 8) | p[k - 1];
    }

    [[nodiscard]] static uint64_t Hash(const void* key, size_t len, uint64_t seed)
    {
        constexpr uint64_t Secrets[] =
        {
            0xa0761d6478bd642full,
            0xe7037ed1a0b428dbull,
            0x8ebc6af09c88c6e3ull,
            0x589965cc75374cc3ull,
        };

        const uint8_t* p = static_cast<const uint8_t*>(key);
        seed ^= Mix(seed ^ Secrets[0], Secrets[1]);

        uint64_t a;
        uint64_t b;

        if (len <= 16) [[likely]]
        {
            if (len >= 4) [[likely]]
            {
                a = (R4(p) << 32) | R4(p + ((len >> 3) << 2));
                b = (R4(p + (len - 4)) << 32) | R4(p + (len - 4 - ((len >> 3) << 2)));
            }
            else if (len > 0) [[likely]]
            {
                a = R3(p, len);
                b = 0;
            }
            else
            {
                a = 0;
                b = 0;
            }
        }
        else
        {
            size_t i = len;
            if (i>48) [[unlikely]]
            {
                uint64_t see1 = seed;
                uint64_t see2 = seed;
                do [[likely]]
                {
                    seed = Mix(R8(p) ^ Secrets[1], R8(p + 8) ^ seed);
                    see1 = Mix(R8(p + 16) ^ Secrets[2], R8(p + 24) ^ see1);
                    see2 = Mix(R8(p + 32) ^ Secrets[3], R8(p + 40) ^ see2);
                    p += 48;
                    i -= 48;
                } while (i > 48);
                seed ^= see1 ^ see2;
            }

            while (i > 16) [[unlikely]]
            {
                seed = Mix(R8(p) ^ Secrets[1], R8(p + 8) ^ seed);
                i -= 16;
                p += 16;
            }
            a = R8(p + (i - 16));
            b = R8(p + (i - 8));
        }

        a ^= Secrets[1];
        b ^= seed;
        Mum(a, b);
        return Mix(a ^ Secrets[0] ^ len, b ^ Secrets[1]);
    }
}

    // --------------------------------------------------------------------------------------------
    WyHash::ValueType WyHash::HashString(const char* str, ValueType seed)
    {
        const uint32_t len = String::Length(str);
        return HashData(str, len, seed);
    }

    WyHash::ValueType WyHash::HashData(const void* data, uint32_t len, ValueType seed)
    {
        return wyhash::Hash(data, len, seed);
    }
}
