// Copyright Chad Engler

#pragma once

#include "he/core/string.h"

namespace he::schema
{
    // Convert a unicode code point into a UTF-8 representation by appending it to a string.
    // Returns the number of bytes generated.
    int ToUTF8(he::String& dst, uint32_t ucc);

    // Converts whatever prefix of the incoming string corresponds to a valid UTF-8 sequence into
    // a unicode code. The incoming pointer will have been advanced past all bytes parsed.
    // Returns -1 upon corrupt UTF-8 encoding (ignore the incoming pointer in this case).
    int FromUTF8(const char*& in);
    bool ValidateUTF8(const char* str);
}
