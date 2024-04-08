// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he
{
    inline bool IsKdlWhitespace(uint32_t ucc)
    {
        return ucc == 0x0009    // Character Tabulation
            || ucc == 0x000b    // Line Tabulation
            || ucc == 0x0020    // Space
            || ucc == 0x00a0    // No-Break Space
            || ucc == 0x1680    // Ogham Space Mark
            || ucc == 0x2000    // En Quad
            || ucc == 0x2001    // Em Quad
            || ucc == 0x2002    // En Space
            || ucc == 0x2003    // Em Space
            || ucc == 0x2004    // Three-Per-Em Space
            || ucc == 0x2005    // Four-Per-Em Space
            || ucc == 0x2006    // Six-Per-Em Space
            || ucc == 0x2007    // Figure Space
            || ucc == 0x2008    // Punctuation Space
            || ucc == 0x2009    // Thin Space
            || ucc == 0x200a    // Hair Space
            || ucc == 0x202f    // Narrow No-Break Space
            || ucc == 0x205f    // Medium Mathematical Space
            || ucc == 0x3000;   // Ideographic Space
    }

    inline bool IsKdlNewline(uint32_t ucc)
    {
        return ucc == 0x000d    // CR  Carriage Return
            || ucc == 0x000a    // LF  Line Feed
            || ucc == 0x0085    // NEL Next Line
            || ucc == 0x000c    // FF  Form Feed
            || ucc == 0x2028    // LS  Line Separator
            || ucc == 0x2029;   // PS  Paragraph Separator
    }

    inline bool IsKdlEqualsSign(uint32_t ucc)
    {
        return ucc == 0x003d    // equals sign (=)
            || ucc == 0xfe66    // small equals sign (﹦)
            || ucc == 0xff1d    // fullwidth equals sign (＝)
            || ucc == 0x1f7f0;  // heavy equals sign (🟰)
    }

    inline bool IsDisallowedKdlCodePoint(uint32_t ucc)
    {
        return (ucc >= 0x00 && ucc <= 0x08)         // various control characters
            || (ucc >= 0x0e && ucc <= 0x1f)         // various control characters
            || (ucc == 0x7f)                        // delete control character (DEL)
            || (ucc >= 0xd800 && ucc <= 0xdfff)     // non unicode scalar values
            || (ucc >= 0x2066 && ucc <= 0x2069)     // directional isolate characters
            || (ucc >= 0x200e && ucc <= 0x200f)     // directional marks
            || (ucc >= 0x202a && ucc <= 0x202e);    // directional control characters
    }

    inline bool IsValidKdlIdentifierCodePoint(uint32_t ucc)
    {
        return ucc != '\\' && ucc != '/'
            && ucc != '(' && ucc != ')'
            && ucc != '{' && ucc != '}'
            && ucc != '[' && ucc != ']'
            && ucc != ';' && ucc != '"' && ucc != '#'
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
