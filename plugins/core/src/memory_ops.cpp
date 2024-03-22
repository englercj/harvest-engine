// Copyright Chad Engler

#include "he/core/memory_ops.h"

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
#if !HE_HAS_LIBC
    // --------------------------------------------------------------------------------------------
    void* MemCopy(void* dst, const void* src, size_t len)
    {
        uint8_t* dst8 = static_cast<uint8_t*>(dst);
        const uint8_t* src8 = static_cast<const uint8_t*>(src);

        typedef uint32_t HE_MAY_ALIAS alias_u32;

        // align the source pointer to a 4 byte boundary
        for (; reinterpret_cast<uintptr_t>(src8) % 4 && len; --len)
            *dst8++ = *src8++;

        // If both the src and dest are 4-byte aligned we can copy over 4 bytes at a time.
        if (reinterpret_cast<uintptr_t>(dst8) % 4 == 0)
        {
            alias_u32* dst32 = reinterpret_cast<alias_u32*>(dst8);
            const alias_u32* src32 = reinterpret_cast<const alias_u32*>(src8);

            for (; len >= 16; len -= 16)
            {
                *dst32++ = *src32++;
                *dst32++ = *src32++;
                *dst32++ = *src32++;
                *dst32++ = *src32++;
            }

            if (len & 8)
            {
                *dst32++ = *src32++;
                *dst32++ = *src32++;
            }

            if (len & 4)
            {
                *dst32++ = *src32++;
            }

            dst8 = reinterpret_cast<uint8_t*>(dst32);
            src8 = reinterpret_cast<const uint8_t*>(src32);

            if (len & 2)
            {
                *dst8++ = *src8++;
                *dst8++ = *src8++;
            }

            if (len & 1)
            {
                *dst8 = *src8;
            }

            return dst;
        }

        if (len >= 32)
        {
        #if HE_CPU_LITTLE_ENDIAN
            #define HE_LS >>
            #define HE_RS <<
        #else
            #define HE_LS <<
            #define HE_RS >>
        #endif
            switch (reinterpret_cast<uintptr_t>(dst8) % 4)
            {
                case 1:
                {
                    uint32_t w = *reinterpret_cast<const alias_u32*>(src8);
                    *dst8++ = *src8++;
                    *dst8++ = *src8++;
                    *dst8++ = *src8++;
                    len -= 3;

                    for (; len >= 17; src8 += 16, dst8 += 16, len -= 16)
                    {
                        uint32_t x = *reinterpret_cast<const alias_u32*>(src8 + 1);
                        *reinterpret_cast<alias_u32*>(dst8 + 0) = (w HE_LS 24) | (x HE_RS 8);
                        w = *reinterpret_cast<const alias_u32*>(src8 + 5);
                        *reinterpret_cast<alias_u32*>(dst8 + 4) = (x HE_LS 24) | (w HE_RS 8);
                        x = *reinterpret_cast<const alias_u32*>(src8 + 9);
                        *reinterpret_cast<alias_u32*>(dst8 + 8) = (w HE_LS 24) | (x HE_RS 8);
                        w = *reinterpret_cast<const alias_u32*>(src8 + 13);
                        *reinterpret_cast<alias_u32*>(dst8 + 12) = (x HE_LS 24) | (w HE_RS 8);
                    }
                    break;
                }
                case 2:
                {
                    uint32_t w = *reinterpret_cast<const alias_u32*>(src8);
                    *dst8++ = *src8++;
                    *dst8++ = *src8++;
                    len -= 2;

                    for (; len >= 18; src8 += 16, dst8 += 16, len -= 16)
                    {
                        uint32_t x = *reinterpret_cast<const alias_u32*>(src8 + 2);
                        *reinterpret_cast<alias_u32*>(dst8 + 0) = (w HE_LS 16) | (x HE_RS 16);
                        w = *reinterpret_cast<const alias_u32*>(src8 + 6);
                        *reinterpret_cast<alias_u32*>(dst8 + 4) = (x HE_LS 16) | (w HE_RS 16);
                        x = *reinterpret_cast<const alias_u32*>(src8 + 10);
                        *reinterpret_cast<alias_u32*>(dst8 + 8) = (w HE_LS 16) | (x HE_RS 16);
                        w = *reinterpret_cast<const alias_u32*>(src8 + 14);
                        *reinterpret_cast<alias_u32*>(dst8 + 12) = (x HE_LS 16) | (w HE_RS 16);
                    }
                    break;
                }
                case 3:
                {
                    uint32_t w = *reinterpret_cast<const alias_u32*>(src8);
                    *dst8++ = *src8++;
                    len -= 1;

                    for (; len >= 19; src8 += 16, dst8 += 16, len -= 16)
                    {
                        uint32_t x = *reinterpret_cast<const alias_u32*>(src8 + 3);
                        *reinterpret_cast<alias_u32*>(dst8 + 0) = (w HE_LS 8) | (x HE_RS 24);
                        w = *reinterpret_cast<const alias_u32*>(src8 + 7);
                        *reinterpret_cast<alias_u32*>(dst8 + 4) = (x HE_LS 8) | (w HE_RS 24);
                        x = *reinterpret_cast<const alias_u32*>(src8 + 11);
                        *reinterpret_cast<alias_u32*>(dst8 + 8) = (w HE_LS 8) | (x HE_RS 24);
                        w = *reinterpret_cast<const alias_u32*>(src8 + 15);
                        *reinterpret_cast<alias_u32*>(dst8 + 12) = (x HE_LS 8) | (w HE_RS 24);
                    }
                    break;
                }
            }
            #undef HE_LS
            #undef HE_RS
        }

        if (len & 16)
        {
            *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++;
            *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++;
            *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++;
            *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++;
        }

        if (len & 8)
        {
            *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++;
            *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++;
        }

        if (len & 4)
        {
            *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++; *dst8++ = *src8++;
        }

        if (len & 2)
        {
            *dst8++ = *src8++;
            *dst8++ = *src8++;
        }

        if (len & 1)
        {
            *dst8 = *src8;
        }

        return dst;
    }

    // --------------------------------------------------------------------------------------------
    void* MemMove(void* dst, const void* src, size_t len)
    {
        uint8_t* dst8 = static_cast<uint8_t*>(dst);
        const uint8_t* src8 = static_cast<const uint8_t*>(src);

        if (dst8 == src8)
            return dst;

        const size_t slack = reinterpret_cast<uintptr_t>(src8) - reinterpret_cast<uintptr_t>(dst8) - len;
        if (slack <= (static_cast<size_t>(-2) * len))
            return MemCopy(dst, src, len);

        constexpr size_t WordSize = sizeof(size_t);

        if (dst8 < src8)
        {
            if ((reinterpret_cast<uintptr_t>(src8) % WordSize) == (reinterpret_cast<uintptr_t>(dst8) % WordSize))
            {
                while (reinterpret_cast<uintptr_t>(dst8) % WordSize)
                {
                    if (len-- == 0)
                        return dst;

                    *dst8++ = *src8++;
                }

                while (len >= WordSize)
                {
                    *reinterpret_cast<size_t* HE_MAY_ALIAS>(dst8) = *reinterpret_cast<const size_t* HE_MAY_ALIAS>(src8);

                    len -= WordSize;
                    dst8 += WordSize;
                    src8 += WordSize;
                }
            }

            while (len)
            {
                *dst8++ = *src8++;
                --len;
            }
        }
        else
        {
            if ((reinterpret_cast<uintptr_t>(src8) % WordSize) == (reinterpret_cast<uintptr_t>(dst8) % WordSize))
            {
                while (reinterpret_cast<uintptr_t>(dst8 + len) % WordSize)
                {
                    if (len-- == 0)
                        return dst;

                    dst8[len] = src8[len];
                }

                while (len >= WordSize)
                {
                    len -= WordSize;
                    *reinterpret_cast<size_t* HE_MAY_ALIAS>(dst8 + len) = *reinterpret_cast<const size_t* HE_MAY_ALIAS>(src8 + len);
                }
            }

            while (len)
            {
                --len;
                dst8[len] = src8[len];
            }
        }

        return dst;
    }

    // --------------------------------------------------------------------------------------------
    int32_t MemCmp(const void* a, const void* b, size_t len)
    {
        const uint8_t* a8 = static_cast<const uint8_t*>(a);
        const uint8_t* b8 = static_cast<const uint8_t*>(b);

        // Loop a word at a time if there are enough bytes and pointers are word aligned.
        if (len >= 4 && IsAligned(a8, 4) && IsAligned(b8, 4))
        {
            while (len >= 4)
            {
                const uint32_t* a32 = reinterpret_cast<const uint32_t*>(a8);
                const uint32_t* b32 = reinterpret_cast<const uint32_t*>(b8);
                if (*a32 != *b32)
                {
                    // Go to the single-byte loop to find the specific byte.
                    break;
                }

                a8 += 4;
                b8 += 4;
                len -= 4;
            }
        }

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

    // --------------------------------------------------------------------------------------------
    void* MemSet(void* mem, int ch, size_t len)
    {
        // Fill head and tail with minimal branching. Each conditional ensures that all the
        // subsequently used offsets are well-defined and in the memory region.
        uint8_t* mem8 = static_cast<uint8_t*>(mem);
        const uint8_t c = static_cast<uint8_t>(ch);

        if (len == 0)
            return mem;

        mem8[0] = c;
        mem8[len - 1] = c;

        if (len <= 2)
            return mem;

        mem8[1] = c;
        mem8[2] = c;
        mem8[len - 2] = c;
        mem8[len - 3] = c;

        if (len <= 6)
            return mem;

        mem8[3] = c;
        mem8[len - 4] = c;

        if (len <= 8)
            return mem;

        // Advance pointer to align it at a 4-byte boundary, and truncate n to a multiple of 4.
        // The previous code already took care of any head/tail that get cut off by the alignment.
        size_t k = -reinterpret_cast<uintptr_t>(mem8) & 3;
        mem8 += k;
        len -= k;
        len &= static_cast<size_t>(-4);

        // In preparation to copy 32 bytes at a time, aligned on an 8-byte boundary, fill head/tail
        // up to 28 bytes each. As in the initial byte-based head/tail fill, each conditional below
        // ensures that the subsequent offsets are valid (e.g. !(len<=24) implies len>=28).
        const uint32_t c32 = (static_cast<uint32_t>(-1) / 255) * c;
        uint32_t* HE_MAY_ALIAS mem32 = reinterpret_cast<uint32_t* HE_MAY_ALIAS>(mem8);

        mem32[0] = c32;
        mem32[len - 1] = c32;

        if (len <= 8)
            return mem;

        mem32[1] = c32;
        mem32[2] = c32;
        mem32[len - 2] = c32;
        mem32[len - 3] = c32;

        if (len <= 24)
            return mem;

        mem32[3] = c32;
        mem32[4] = c32;
        mem32[5] = c32;
        mem32[6] = c32;
        mem32[len - 4] = c32;
        mem32[len - 5] = c32;
        mem32[len - 6] = c32;
        mem32[len - 7] = c32;

        // Align to a multiple of 8 so we can fill 64 bits at a time, and avoid writing the same
        // bytes twice as much as is practical without introducing additional branching.
        k = 24 + (reinterpret_cast<uintptr_t>(mem8) & 4);
        mem8 += k;
        len -= k;

        // If this loop is reached, 28 tail bytes have already been filled, so any remainder when
        // `len` drops below 32 can be safely ignored.
        const uint32_t c64 = (static_cast<uint64_t>(c32) << 32) | c32;
        uint64_t* HE_MAY_ALIAS mem64 = reinterpret_cast<uint64_t* HE_MAY_ALIAS>(mem8);

        for (; len >= 32; len -= 32, mem64 += 4)
        {
            mem64[0] = c64;
            mem64[1] = c64;
            mem64[2] = c64;
            mem64[3] = c64;
        }

        return mem;
    }

    // --------------------------------------------------------------------------------------------
    const void* MemChr(const void* mem, int ch, size_t len)
    {
        constexpr size_t Alignment = sizeof(size_t);
        constexpr size_t Ones = static_cast<size_t>(-1) / 0xff;
        constexpr size_t Highs = Ones * ((0xff / 2) + 1);

        #define HE_MEMCHR_HAS_ZERO(x) (((x) - Ones) & (~(x) & Highs))

        const uint8_t* mem8 = static_cast<const uint8_t*>(mem);
        const uint8_t c = static_cast<uint8_t>(ch);

        if (IsAligned(mem8, Alignment))
        {
            while (len)
            {
                if (*mem8 == c)
                    break;

                ++mem8;
                --len;
            }

            if (len && *mem8 != c)
            {
                const size_t k = Ones * c;
                const size_t* HE_MAY_ALIAS w = reinterpret_cast<const size_t* HE_MAY_ALIAS>(mem8);
                for (; len >= Alignment; ++w, len -= Alignment)
                {
                    if (HE_MEMCHR_HAS_ZERO(*w ^ k))
                    {
                        break;
                    }
                }
                mem8 = reinterpret_cast<const uint8_t*>(w);
            }
        }

        #undef HE_MEMCHR_HAS_ZERO

        while (len)
        {
            if (*mem8 == c)
                return mem8;

            ++mem8;
            --len;
        }

        return nullptr;
    }
#endif
}
