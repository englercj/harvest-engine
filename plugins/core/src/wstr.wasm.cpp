// Copyright Chad Engler

#include "he/core/wstr.h"

#include "he/core/assert.h"

#if defined(HE_PLATFORM_API_POSIX)

namespace he
{
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

        // TODO
    }

    uint32_t WCToMBStr(char* dst, uint32_t dstLen, const wchar_t* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        // No destination pointer, just count the number of bytes needed
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
                    return static_cast<uint32_t>(-1);

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
                    return static_cast<uint32_t>(-1);

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
