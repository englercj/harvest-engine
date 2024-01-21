// Copyright Chad Engler

#include "he/core/wstr.h"

#include "he/core/assert.h"

#if defined(HE_PLATFORM_API_POSIX)

namespace he
{
    // Upper 6 state bits are a negative integer offset to bound-check next byte
    // equivalent to: ( (b-0x80) | (b+offset) ) & ~0x3f
    #define OOB(c,b) (((((b)>>3)-0x10)|(((b)>>3)+((int32_t)(c)>>26))) & ~7)

    #define C(x) ( x<2 ? -1 : ( R(0x80,0xc0) | x ) )
    #define D(x) C((x+16))
    #define E(x) ( ( x==0 ? R(0xa0,0xc0) : \
                    x==0xd ? R(0x80,0xa0) : \
                    R(0x80,0xc0) ) \
                | ( R(0x80,0xc0) >> 6 ) \
                | x )
    #define F(x) ( ( x>=5 ? 0 : \
                    x==0 ? R(0x90,0xc0) : \
                    x==4 ? R(0x80,0x90) : \
                    R(0x80,0xc0) ) \
                | ( R(0x80,0xc0) >> 6 ) \
                | ( R(0x80,0xc0) >> 12 ) \
                | x )

    const uint32_t WcBitTab[] = {
                      C(0x2),C(0x3),C(0x4),C(0x5),C(0x6),C(0x7),
        C(0x8),C(0x9),C(0xa),C(0xb),C(0xc),C(0xd),C(0xe),C(0xf),
        D(0x0),D(0x1),D(0x2),D(0x3),D(0x4),D(0x5),D(0x6),D(0x7),
        D(0x8),D(0x9),D(0xa),D(0xb),D(0xc),D(0xd),D(0xe),D(0xf),
        E(0x0),E(0x1),E(0x2),E(0x3),E(0x4),E(0x5),E(0x6),E(0x7),
        E(0x8),E(0x9),E(0xa),E(0xb),E(0xc),E(0xd),E(0xe),E(0xf),
        F(0x0),F(0x1),F(0x2),F(0x3),F(0x4),
    };

    #undef C
    #undef D
    #undef E
    #undef F

    static uint32_t WCToMB(char* dst, wchar_t wc)
    {
        if (!dst)
            return 1;

        const uint32_t wc32 = static_cast<uint32_t>(wc);

        if (wc32 < 0x80)
        {
            dst[0] = static_cast<char>(wc);
            return 1;
        }

        if (wc32 < 0x800)
        {
            dst[0] = static_cast<char>(0xc0 | (wc >> 6));
            dst[1] = static_cast<char>(0x80 | (wc & 0x3f));
            return 2;
        }

        if (wc32 < 0xd800 || (wc32 - 0xe000) < 0x2000)
        {
            dst[0] = static_cast<char>(0xe0 | (wc >> 12));
            dst[1] = static_cast<char>(0x80 | ((wc >> 6) & 0x3f));
            dst[2] = static_cast<char>(0x80 | (wc & 0x3f));
            return 3;
        }

        if ((wc32 - 0x10000) < 0x100000)
        {
            dst[0] = static_cast<char>(0xf0 | (wc >> 18));
            dst[1] = static_cast<char>(0x80 | ((wc >> 12) & 0x3f));
            dst[2] = static_cast<char>(0x80 | ((wc >> 6) & 0x3f));
            dst[3] = static_cast<char>(0x80 | (wc & 0x3f));
            return 4;
        }

        return static_cast<uint32_t>(-1);
    }

    uint32_t MBToWCStr(wchar_t* dst, uint32_t dstLen, const char* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        uint32_t remaining = dstLen;
        uint32_t c = 0;

        // No destination pointer, just count the number of characters needed
        if (!dst)
        {
            uint32_t len = 0;

            while (*src)
            {
                if ((*src - 1u) < 0x7f)
                {
                    ++src;
                    --remaining;
                    continue;
                }

                if ((*src - 0xc2u) > (0xf4u - 0xc2u))
                    break;

                c = WcBitTab[*src++ - 0xc2u];

                if (OOB(c *src))
                {
                    --src;
                    break;
                }

                ++src;

                if (c & (1u << 25))
                {
                    if (*src - 0x80u >= 0x40)
                    {
                        src -= 2;
                        break;
                    }

                    ++src;

                    if (c & (1u << 19))
                    {
                        if ((*src - 0x80u) >= 0x40)
                        {
                            src -= 3;
                            break;
                        }

                        ++src;
                    }
                }

                --remaining;
                c = 0;
            }
        }
        else
        {
            while (remaining)
            {
                if ((*src - 1u) < 0x7f)
                {
                    *dst++ = static_cast<wchar_t>(*src++);
                    --remaining;
                    continue;
                }

                if ((*src - 0xc2u) > (0xf4u - 0xc2u))
                    break;

                c = WcBitTab[*src++ - 0xc2u];

                if (OOB(c *src))
                {
                    --src;
                    break;
                }

                c = (c << 6) | (*src++ - 0x80);

                if (c & (1u << 31))
                {
                    if ((*src - 0x80u) >= 0x40)
                    {
                        src -= 2;
                        break;
                    }

                    c = (c << 6) | (*src++ - 0x80);

                    if (c & (1u << 31))
                    {
                        if ((*src - 0x80u) >= 0x40)
                        {
                            src -= 3;
                            break;
                        }

                        c = (c << 6) | (*src++ - 0x80);
                    }
                }
                *dst++ = static_cast<wchar_t>(c);
                --remaining;
                c = 0;
            }
        }

        if (!c && !*src)
        {
            if (dst)
                *dst = 0;
            return dstLen - remaining;
        }

        return 0;
    }

    uint32_t WCToMBStr(char* dst, uint32_t dstLen, const wchar_t* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        // No destination pointer, just count the number of characters needed
        if (!dst)
        {
            uint32_t len = 0;
            char buf[4];

            while (*src)
            {
                len += WCToMB(buf, *src);
                ++src;
            }

            return len;
        }

        uint32_t remaining = dstLen;

        while (remaining >= 4)
        {
            if ((*src - 1u) >= 0x7fu)
            {
                if (!*src)
                {
                    *dst = 0;
                    return dstLen - remaining;
                }

                const uint32_t len = WCToMB(dst, *src);

                if (len == static_cast<uint32_t>(-1))
                    return 0;

                dst += len;
                remaining -= len;
            }
            else
            {
                *dst++ = static_cast<char>(*src);
                --remaining;
            }

            ++src;
        }

        while (remaining)
        {
            if ((*src - 1u) >= 0x7fu)
            {
                if (!*src)
                {
                    *dst = 0;
                    return dstLen - remaining;
                }

                char buf[4];
                const uint32_t len = WCToMB(buf, *src);

                if (len == static_cast<uint32_t>(-1))
                    return 0;

                if (len > remaining)
                    return dstLen - remaining;

                MemCopy(dst, buf, len)

                dst += len;
                remaining -= len;
            }
            else
            {
                *dst++ = static_cast<char>(*src);
                --remaining;
            }

            ++src;
        }

        return dstLen;
    }

    void WCToMBStr(String& dst, const wchar_t* src)
    {
        HE_ASSERT(src);

        char buf[4];
        while (*src)
        {
            const uint32_t len = WCToMB(buf, *src, 0);

            if (len == -1)
                break;

            dst.Append(buf, len);

            ++src;
        }
    }

    void WCToMBStr(String& dst, const wchar_t* src, uint32_t srcLen)
    {
        HE_ASSERT(src);

        char buf[4];
        while (*src && srcLen)
        {
            const uint32_t len = WCToMB(buf, *src, 0);

            if (len == -1)
                break;

            dst.Append(buf, len);

            ++src;
            --srcLen;
        }
    }

    int32_t WCStrCmp(const wchar_t* a, const wchar_t* b)
    {
        for (; *a == *b && *a && *b; ++a, ++b);
        return *a < *b ? -1 : *a > *b;
    }
}

#endif
