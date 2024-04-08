// Copyright Chad Engler

#include "he/core/utf8.h"

#include "he/core/compiler.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include "simdutf.h"

namespace he
{
    inline bool IsUTF8Continuation(char ch) { return (static_cast<uint8_t>(ch) & 0xc0) == 0x80; }

    uint32_t UTF8Encode(he::String& dst, uint32_t ucc)
    {
        // Top bit can't be set.
        if ((ucc & 0x80000000) != 0)
            return 0;

        // 6 possible encodings: http://en.wikipedia.org/wiki/UTF-8
        for (uint32_t i = 0; i < 6; ++i)
        {
            // Max bits this encoding can represent.
            uint32_t max_bits = 6 + (i * 5) + static_cast<uint32_t>(!i);

            if (ucc < (1u << max_bits))
            {
                // Remaining bits not encoded in the first byte, store 6 bits each
                uint32_t remain_bits = i * 6;

                // Store bytes
                dst += static_cast<char>((0xFE << (max_bits - remain_bits)) | (ucc >> remain_bits));
                for (uint32_t j = i - 1; j >= 0; --j)
                {
                    dst += static_cast<char>(((ucc >> (j * 6)) & 0x3F) | 0x80);
                }

                // Return the number of bytes added.
                return i + 1;
            }
        }

        return 0;
    }

    uint32_t UTF8Decode(uint32_t& dst, const char* str, uint32_t len)
    {
        dst = InvalidCodePoint;

        const char* end = str + len;

        // Strings too small, simply parse zero bytes.
        if (str >= end)
            return 0;

        const uint8_t ch = static_cast<uint8_t>(*str);

        // ASCII code point
        if ((ch & 0x80) == 0)
        {
            dst = ch;
            return 1;
        }

        // 2-byte sequence
        if ((ch & 0xe0) == 0xc0)
        {
            if (str + 1 >= end)
                return 0;

            if (!IsUTF8Continuation(str[1]))
                return InvalidCodePoint;

            dst = ((ch & 0x1f) << 6) | (str[1] & 0x3f);
            return 2;
        }

        // 3-byte sequence
        if ((ch & 0xf0) == 0xe0)
        {
            if (str + 2 >= end)
                return 0;

            if (!IsUTF8Continuation(str[1]) || !IsUTF8Continuation(str[2]))
                return InvalidCodePoint;

            dst = ((ch & 0x0f) << 12) | ((str[1] & 0x3f) << 6) | (str[2] & 0x3f);
            return 3;
        }

        // 4-byte sequence
        if ((ch & 0xf8) == 0xf0)
        {
            if (str + 3 >= end)
                return 0;

            if (!IsUTF8Continuation(str[1]) || !IsUTF8Continuation(str[2]) || !IsUTF8Continuation(str[3]))
                return InvalidCodePoint;

            dst = ((ch & 0x07) << 18) | ((str[1] & 0x3f) << 12) | ((str[2] & 0x3f) << 6) | (str[3] & 0x3f);
            return 4;
        }

        return InvalidCodePoint;
    }

    bool UTF8Validate(const char* str, uint32_t len)
    {
        return simdutf::validate_utf8(str, len);
    }

    uint32_t UTF8Length(const char* str, uint32_t len)
    {
        return static_cast<uint32_t>(simdutf::count_utf8(str, len));
    }
}
