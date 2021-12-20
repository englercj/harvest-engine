// Copyright Chad Engler

#include "utf8_helpers.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"

namespace he::schema
{
    int ToUTF8(String& dst, uint32_t ucc)
    {
        // Top bit can't be set.
        HE_ASSERT((ucc & 0x80000000) == 0);

        // 6 possible encodings: http://en.wikipedia.org/wiki/UTF-8
        for (int i = 0; i < 6; ++i)
        {
            // Max bits this encoding can represent.
            uint32_t max_bits = 6 + (i * 5) + static_cast<uint32_t>(!i);

            if (ucc < (1u << max_bits))
            {
                // Remaining bits not encoded in the first byte, store 6 bits each
                uint32_t remain_bits = i * 6;

                // Store bytes
                dst += static_cast<char>((0xFE << (max_bits - remain_bits)) | (ucc >> remain_bits));
                for (int j = i - 1; j >= 0; --j)
                {
                    dst += static_cast<char>(((ucc >> (j * 6)) & 0x3F) | 0x80);
                }

                // Return the number of bytes added.
                return i + 1;
            }
        }

        HE_ASSERT(false, "Invalid unicode code point");
        HE_UNREACHABLE();
    }

    int FromUTF8(const char*& in)
    {
        int len = 0;

        // Count leading 1 bits.
        for (uint32_t mask = 0x80; mask >= 0x04; mask >>= 1)
        {
            if (*in & mask)
                ++len;
            else
                break;
        }

        // Bit after leading 1's must be 0.
        if ((static_cast<uint8_t>(*in) << len) & 0x80)
            return -1;

        if (!len)
            return *in++;

        // UTF-8 encoded values with a length are between 2 and 4 bytes.
        if (len < 2 || len > 4)
            return -1;

        // Grab initial bits of the code.
        int ucc = *in++ & ((1 << (7 - len)) - 1);

        for (int i = 0; i < len - 1; i++)
        {
            // Upper bits must 1 0.
            if ((*in & 0xC0) != 0x80)
                return -1;

            ucc <<= 6;
            ucc |= *in++ & 0x3F; // Grab 6 more bits of the code.
        }

        // UTF-8 cannot encode values between 0xD800 and 0xDFFF (reserved for UTF-16 surrogate pairs).
        if (ucc >= 0xD800 && ucc <= 0xDFFF)
            return -1;

        // UTF-8 must represent code points in their shortest possible encoding.
        switch (len)
        {
            case 2:
                // Two bytes of UTF-8 can represent code points from U+0080 to U+07FF.
                if (ucc < 0x0080 || ucc > 0x07FF)
                    return -1;
                break;
            case 3:
                // Three bytes of UTF-8 can represent code points from U+0800 to U+FFFF.
                if (ucc < 0x0800 || ucc > 0xFFFF)
                    return -1;
                break;
            case 4:
                // Four bytes of UTF-8 can represent code points from U+10000 to U+10FFFF.
                if (ucc < 0x10000 || ucc > 0x10FFFF)
                    return -1;
                break;
        }

        return ucc;
    }

    bool ValidateUTF8(const char* str)
    {
        while (*str)
        {
            if (FromUTF8(str) < 0)
                return false;
        }

        return true;
    }
}
