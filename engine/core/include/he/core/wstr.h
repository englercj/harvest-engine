// Copyright Chad Engler

#include "he/core/alloca.h"

// Due to how the TO_WSTR macro is setup it isn't easy to pass along the size of the wstr buffer
// that gets allocated. So instead we specify a huge size as our buffer size for MultiByteToWideChar.
// This is safe because the wstr buffer is actually large enough to fit all the converted
// characters.

/// Converts a multibyte character string to a wide character string.
/// The destination wide character string is allocated on the stack using HE_ALLOCA,
/// so no free operation is necessary.
///
/// \param Str The multibyte character string to convert.
/// \return The wide character version of the string.
#define HE_TO_WSTR(STR) ([](wchar_t* wstr, const char* str) { \
    (void)MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, 0x3fffffff); \
    return wstr; \
}(HE_ALLOCA(wchar_t, MultiByteToWideChar(CP_UTF8, 0, STR, -1, nullptr, 0)), STR))
