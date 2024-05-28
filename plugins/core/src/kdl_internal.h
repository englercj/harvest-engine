// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he
{
    inline bool IsKdlWhitespace(uint32_t ucc)
    {
        switch (ucc)
        {
            case 0x0009:    // Character Tabulation
            case 0x000b:    // Line Tabulation
            case 0x0020:    // Space
            case 0x00a0:    // No-Break Space
            case 0x1680:    // Ogham Space Mark
            case 0x2000:    // En Quad
            case 0x2001:    // Em Quad
            case 0x2002:    // En Space
            case 0x2003:    // Em Space
            case 0x2004:    // Three-Per-Em Space
            case 0x2005:    // Four-Per-Em Space
            case 0x2006:    // Six-Per-Em Space
            case 0x2007:    // Figure Space
            case 0x2008:    // Punctuation Space
            case 0x2009:    // Thin Space
            case 0x200a:    // Hair Space
            case 0x202f:    // Narrow No-Break Space
            case 0x205f:    // Medium Mathematical Space
            case 0x3000:    // Ideographic Space
                return true;
        }

        return false;
    }

    inline bool IsKdlNewline(uint32_t ucc)
    {
        switch (ucc)
        {
            case 0x000d:    // CR  Carriage Return
            case 0x000a:    // LF  Line Feed
            case 0x0085:    // NEL Next Line
            case 0x000c:    // FF  Form Feed
            case 0x2028:    // LS  Line Separator
            case 0x2029:    // PS  Paragraph Separator
                return true;
        }

        return false;
    }

    inline bool IsKdlEqualsSign(uint32_t ucc)
    {
        switch (ucc)
        {
            case 0x003d:    // equals sign (=)
            case 0xfe66:    // small equals sign (﹦)
            case 0xff1d:    // fullwidth equals sign (＝)
            case 0x1f7f0:   // heavy equals sign (🟰)
                return true;
        }

        return false;
    }

    inline bool IsDisallowedKdlCodePoint(uint32_t ucc)
    {
        return (ucc >= 0x00 && ucc <= 0x08)         // various control characters
            || (ucc >= 0x0e && ucc <= 0x1f)         // various control characters
            || (ucc == 0x7f)                        // delete control character (DEL)
            || (ucc >= 0xd800 && ucc <= 0xdfff)     // non unicode scalar values
            || (ucc >= 0x2066 && ucc <= 0x2069)     // directional isolate characters
            || (ucc >= 0x200e && ucc <= 0x200f)     // directional marks
            || (ucc >= 0x202a && ucc <= 0x202e)     // directional control characters
            || (ucc == 0xfeff);                     // zero width no-break space / Byte Order Mark
    }

    inline bool IsKdlUnicodeScalarValue(uint32_t ucc)
    {
        return (ucc >= 0x0000 && ucc <= 0xd7ff) || (ucc >= 0xe000 && ucc <= 0x10ffff);
    }

    inline bool IsValidKdlIdentifierCodePoint(uint32_t ucc)
    {
        switch (ucc)
        {
            case '\\':
            case '/':
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case ';':
            case '"':
            case '#':
                return false;
        }

        return IsKdlUnicodeScalarValue(ucc)
            && !IsDisallowedKdlCodePoint(ucc)
            && !IsKdlEqualsSign(ucc)
            && !IsKdlWhitespace(ucc)
            && !IsKdlNewline(ucc);
    }

    inline bool IsValidKdlIdentifierStartCodePoint(uint32_t ucc)
    {
        return IsValidKdlIdentifierCodePoint(ucc) && (ucc < '0' || ucc > '9');
    }
}
