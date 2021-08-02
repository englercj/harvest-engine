// Copyright Chad Engler

#pragma once

#include "he/core/alloca.h"

// Due to how the TO_WSTR macro is setup it isn't easy to pass along the size of the wstr buffer
// that gets allocated. So instead we specify a huge size as our buffer size for MultiByteToWideChar.
// This is safe because the wstr buffer is actually large enough to fit all the converted
// characters.

/// Converts a multibyte character string to a wide character string.
/// The destination wide character string is allocated on the stack using HE_ALLOCA,
/// so no free operation is necessary.
///
/// \param STR The multibyte character string to convert.
/// \return The wide character version of the string.
#define HE_TO_WSTR(STR) ([](wchar_t* dst, const char* src) { \
    (void)MBToWCStr(dst, 0x3fffffff, src); \
    return dst; \
}(HE_ALLOCA(wchar_t, MBToWCStr(nullptr, 0, STR)), STR))

namespace he
{
    /// Converts a null terminated multibyte UTF-8 string to a wide character string.
    ///
    /// \note
    /// Behavior is undefined if any of the following are true:
    /// - `src` is null
    /// - `dst` is non-null and `dstLen` is zero
    /// - `dst` is null and `dstLen` is non-zero.
    ///
    /// \param dst The wide character buffer to write the result to.
    /// \param dstLen The length of the destination buffer in wide characters.
    /// \param src The source string to convert.
    /// \return The number of bytes written to the destination buffer (including the null
    /// terminator). If `dst` is null and `dstLen` is zero then the number of characters
    /// that would be written assuming an infinite buffer are returned, including the null
    /// terminator.
    uint32_t MBToWCStr(wchar_t* dst, uint32_t dstLen, const char* src);

    /// Converts a null terminated wide character UTF-8 string to a multibyte string.
    ///
    /// \note
    /// Behavior is undefined if any of the following are true:
    /// - `src` is null
    /// - `dst` is non-null and `dstLen` is zero
    /// - `dst` is null and `dstLen` is non-zero.
    ///
    /// \param dst The character buffer to write the result to.
    /// \param dstLen The length of the destination buffer in characters.
    /// \param src The source wide string to convert.
    /// \return The number of bytes written to the destination buffe, including the null
    /// terminator. If `dst` is null and `dstLen` is zero then the number of characters
    /// that would be written assuming an infinite buffer are returned, including the null
    /// terminator.
    uint32_t WCToMBStr(char* dst, uint32_t dstLen, const wchar_t* src);

    /// Compares the null terminated wide character strings and returns the result of the comparison.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return The result of the comparison.
    ///     If the values are equal, zero is returned.
    ///     If this string is less than `x`, a negative value is returned.
    ///     If this string is greater than `x`, a positive value is returned.
    int32_t WCStrCmp(const wchar_t* a, const wchar_t* b);
}
