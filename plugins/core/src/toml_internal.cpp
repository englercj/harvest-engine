#include "toml_internal.h"

namespace he
{
    bool IsValidTomlKeyCodePoint(uint32_t ucc)
    {
        // a-z A-Z 0-9 - _
        return (ucc >= 'a' && ucc <= 'z')
            || (ucc >= 'A' && ucc <= 'Z')
            || (ucc >= '0' && ucc <= '9')
            || (ucc == '_' || ucc == '-')
            // non-symbol chars in Latin block
            || (ucc >= 0xc0 && ucc <= 0xd6)
            || (ucc >= 0xd8 && ucc <= 0xf6)
            || (ucc >= 0xf8 && ucc <= 0xff)
            // this excludes GREEK QUESTION MARK, which is basically a semi-colon
            || (ucc >= 0x0100 && ucc <= 0x02ff)
            || (ucc >= 0x0300 && ucc <= 0x037d)
            || (ucc >= 0x037f && ucc <= 0x1fff)
            // include combining chars used in some languages
            || (ucc >= 0x200c && ucc <= 0x200d)
            || (ucc >= 0x203f && ucc <= 0x2040)
            // this excludes arrows, blocks and the like
            || (ucc >= 0x2070 && ucc <= 0x218f)
            || (ucc >= 0x2c00 && ucc <= 0x2fef)
            // skip 2FF0-3000 ideographic up/down markers and spaces
            || (ucc >= 0x3001 && ucc <= 0xd7ff)
            // skip D800-D999 surrogate block, E000-F8FF Private Use area,
            // FDD0-FDEF intended for process-internal use (unicode)
            || (ucc >= 0xf900 && ucc <= 0xfdcf)
            || (ucc >= 0xfdf0 && ucc <= 0xfffd)
            // all chars outside BMP range, excluding Private Use planes
            || (ucc >= 0x10000 && ucc <= 0xeffff);
    }
}
