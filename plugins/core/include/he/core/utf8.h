// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    /// Convert a unicode code point into a UTF-8 representation by appending it to a string.
    ///
    /// \param[out] dst The destination string to add the code point to.
    /// \param[in] ucc The unicode code point to append.
    /// \return Returns the number of bytes generated.
    uint32_t ToUTF8(he::String& dst, uint32_t ucc);

    /// Converts whatever prefix of the incoming string corresponds to a valid UTF-8 sequence into
    /// a unicode code point. The incoming pointer will have been advanced past all bytes parsed.
    ///
    /// \param[in,out] str The incoming string to parse and advanced past the parsed bytes.
    /// \return If the UTF-8 encoded string is valid, returns the unicode code point and advanced
    /// the pointer past the parsed bytes. Otherwise, returns -1 and the pointer is not modified.
    uint32_t FromUTF8(const char*& str);

    /// Validates that the incoming string is a valid UTF-8 sequence.
    ///
    /// \param[in] str The string to verify.
    /// \return True if the string is valid, false otherwise.
    bool ValidateUTF8(const char* str);
}
