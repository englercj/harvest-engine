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
        char buf[6];
        const uint32_t len = UTF8Encode(buf, ucc);

        if (len)
        {
            dst.Append(buf, len);
        }
        return len;
    }

    uint32_t UTF8Encode(char dst[6], uint32_t ucc)
    {
        // Top bit can't be set.
        if ((ucc & 0x80000000) != 0)
            return 0;

        // 6 possible encodings: http://en.wikipedia.org/wiki/UTF-8
        for (uint32_t i = 0; i < 6; ++i)
        {
            // Max bits this encoding can represent.
            uint32_t maxBits = 6 + (i * 5) + static_cast<uint32_t>(!i);

            if (ucc < (1u << maxBits))
            {
                // Remaining bits not encoded in the first byte, store 6 bits each
                uint32_t remainingBits = i * 6;

                // Store bytes
                *dst++ = static_cast<char>((0xFE << (maxBits - remainingBits)) | (ucc >> remainingBits));
                for (uint32_t j = i - 1; j != static_cast<uint32_t>(-1); --j)
                {
                    *dst++ = static_cast<char>(((ucc >> (j * 6)) & 0x3F) | 0x80);
                }

                // Return the number of bytes written.
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

    bool UTF8IsWhitespace(uint32_t ucc)
    {
        switch (ucc)
        {
            case 0x0009: // Character Tabulation
            case 0x000a: // Line Feed
            case 0x000b: // Line Tabulation
            case 0x000c: // Form Feed
            case 0x000d: // Carriage Return
            case 0x0020: // Space
            case 0x0085: // Next Line
            case 0x00a0: // No-Break Space
            case 0x1680: // Ogham Space Mark
            case 0x2000: // En Quad
            case 0x2001: // Em Quad
            case 0x2002: // En Space
            case 0x2003: // Em Space
            case 0x2004: // Three-Per-Em Space
            case 0x2005: // Four-Per-Em Space
            case 0x2006: // Six-Per-Em Space
            case 0x2007: // Figure Space
            case 0x2008: // Punctuation Space
            case 0x2009: // Thin Space
            case 0x200a: // Hair Space
            case 0x2028: // Line Separator
            case 0x2029: // Paragraph Separator
            case 0x202f: // Narrow No-Break Space
            case 0x205f: // Medium Mathematical Space
            case 0x3000: // Ideographic Space
                return true;
        }

        return false;
    }
}
