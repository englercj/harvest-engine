// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/types.h"

namespace he
{
    /// Checks if a string is nullptr or the first character is a null terminator.
    ///
    /// \param s The string to check.
    /// \return True when `s` is null or empty.
    [[nodiscard]] constexpr bool StrEmpty(const char* s);

    /// Gets the length of a null terminated string using a constexpr algorithm. This may be
    /// slower than \see Length(const char*) for cases that are not calculated at compile time.
    ///
    /// \param s The string to get the length of.
    /// \return The length of the string.
    [[nodiscard]] constexpr uint32_t StrLenConst(const char* s);

    /// Gets the length of a null terminated string.
    ///
    /// \param s The string to get the length of.
    /// \return The length of the string.
    [[nodiscard]] uint32_t StrLen(const char* s);

    /// Gets the length of a null terminated string, up to `len`.
    ///
    /// \param s The string to get the length of.
    /// \param len The maximum length of the string to check.
    /// \return The length of the string.
    [[nodiscard]] uint32_t StrLenN(const char* s, uint32_t len);

    /// Compares the null terminated strings and returns the result of the comparison.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return The result of the comparison.
    ///     If the values are equal, zero is returned.
    ///     If this string is less than `x`, a negative value is returned.
    ///     If this string is greater than `x`, a positive value is returned.
    [[nodiscard]] int32_t StrComp(const char* a, const char* b);

    /// Compares the null terminated strings, up to `len`, and returns the result of the
    /// comparison.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \param len The maximum length of the string to check.
    /// \return The result of the comparison.
    ///     If the values are equal, zero is returned.
    ///     If this string is less than `x`, a negative value is returned.
    ///     If this string is greater than `x`, a positive value is returned.
    [[nodiscard]] int32_t StrCompN(const char* a, const char* b, uint32_t len);

    /// Compares the null terminated strings in a case-insensitive manner and returns the
    /// result of the comparison.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return The result of the comparison.
    ///     If the values are equal, zero is returned.
    ///     If this string is less than `x`, a negative value is returned.
    ///     If this string is greater than `x`, a positive value is returned.
    [[nodiscard]] int32_t StrCompI(const char* a, const char* b);

    /// Compares the null terminated strings, up to `len`, in a case-insensitive manner and
    /// returns the result of the comparison.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \param len The maximum length of the string to check.
    /// \return The result of the comparison.
    ///     If the values are equal, zero is returned.
    ///     If this string is less than `x`, a negative value is returned.
    ///     If this string is greater than `x`, a positive value is returned.
    [[nodiscard]] int32_t StrCompNI(const char* a, const char* b, uint32_t len);

    /// Duplicate the null terminated source string using the provided allocator. The
    /// resulting string must be freed by calling \ref Allocator::Free with the same
    /// allocator used here.
    ///
    /// \param[in] allocator The allocator to use for creating the new string.
    /// \param[in] src The source string to duplicate.
    /// \return The newly allocated string.
    [[nodiscard]] char* StrDup(const char* src, Allocator& allocator = Allocator::GetDefault());

    /// Duplicate `len` characters of the source string using the provided allocator. The
    /// resulting string must be freed by calling \ref Allocator::Free with the same
    /// allocator used here.
    ///
    /// \param[in] allocator The allocator to use for creating the new string.
    /// \param[in] src The source string to duplicate.
    /// \param[in] len The length of the source string to duplicate.
    /// \return The newly allocated string.
    [[nodiscard]] char* StrDupN(const char* src, uint32_t len, Allocator& allocator = Allocator::GetDefault());

    /// Compares the null terminated strings and returns true if they are equal.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return True if the strings are equal, false otherwise.
    [[nodiscard]] bool StrEqual(const char* a, const char* b);

    /// Compares the null terminated strings, up to `len`, and returns true if they are equal.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return True if the strings are equal, false otherwise.
    [[nodiscard]] bool StrEqualN(const char* a, const char* b, uint32_t len);

    /// Compares the null terminated strings in a case-insensitive manner and
    /// returns true if they are equal.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return True if the strings are equal, false otherwise.
    [[nodiscard]] bool StrEqualI(const char* a, const char* b);

    /// Compares the null terminated strings, up to `len`, in a case-insensitive manner and
    /// returns true if they are equal.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return True if the strings are equal, false otherwise.
    [[nodiscard]] bool StrEqualNI(const char* a, const char* b, uint32_t len);

    /// Compares the null terminated strings and returns true if `a` is less than `b`.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return True if `a` is less than `b`, false otherwise.
    [[nodiscard]] bool StrLess(const char* a, const char* b);

    /// Compares the null terminated strings, up to `len`, and returns true if `a` is less than `b`.
    ///
    /// \param a The left-hand side of the comparison operation.
    /// \param b The right-hand side of the comparison operation.
    /// \return True if `a` is less than `b`, false otherwise.
    [[nodiscard]] bool StrLessN(const char* a, const char* b, uint32_t len);

    /// Copies the source string into the destination buffer, including the null terminator.
    /// The destination buffer is guaranteed to be null terminated if `dstLen > 0`.
    ///
    /// \param dst The destination buffer to copy into.
    /// \param dstLen The size of the destination buffer.
    /// \param src The string to copy from.
    /// \return The number of characters copied into `dst`, excluding the null terminator.
    uint32_t StrCopy(char* dst, uint32_t dstLen, const char* src);

    /// \copydoc Copy(char*, uint32_t, const char*)
    template <uint32_t N>
    uint32_t StrCopy(char (&dst)[N], const char* src);

    /// Copies up to `srcLen` characters from the source string into the destination buffer,
    /// including the null terminator. The destination buffer is guaranteed to be null
    /// terminated if `dstLen > 0`.
    ///
    /// \param dst The destination buffer to copy into.
    /// \param dstLen The size of the destination buffer.
    /// \param src The string to copy from.
    /// \param srcLen The maximum number of characters to copy.
    /// \return The number of characters copied into `dst`, excluding the null terminator.
    uint32_t StrCopyN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen);

    /// \copydoc CopyN(char*, uint32_t, const char*, uint32_t)
    template <uint32_t N>
    uint32_t StrCopyN(char (&dst)[N], const char* src, uint32_t srcLen);

    /// Copies the source string to the end of the destination string.
    /// The destination buffer is guaranteed to be null terminated if `dstLen > 0`.
    ///
    /// \param dst The destination buffer to copy into.
    /// \param dstLen The size of the destination buffer.
    /// \param src The string to copy from.
    /// \return The length of the `dst` string after the concatenation completes.
    uint32_t StrCat(char* dst, uint32_t dstLen, const char* src);

    /// \copydoc Cat(char*, uint32_t, const char*)
    template <uint32_t N>
    uint32_t StrCat(char (&dst)[N], const char* src);

    /// Copies up to `srcLen` characters from the source string to the end of the destination
    /// string. The destination buffer is guaranteed to be null terminated if `dstLen > 0`.
    ///
    /// \param dst The destination buffer to copy into.
    /// \param dstLen The size of the destination buffer.
    /// \param src The string to copy from.
    /// \param srcLen The maximum number of characters to copy.
    /// \return The length of the `dst` string after the concatenation completes.
    uint32_t StrCatN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen);

    /// \copydoc CatN(char*, uint32_t, const char*, uint32_t)
    template <uint32_t N>
    uint32_t StrCatN(char (&dst)[N], const char* src, uint32_t srcLen);

    /// Searches the null terminated string for a character. Behavior is undefined if `str`
    /// is nullptr.
    ///
    /// \param str The string to search within.
    /// \param search The character to search for.
    /// \return A pointer to the found character in `str`, or nullptr if not found.
    [[nodiscard]] const char* StrFind(const char* str, char search);

    /// Searches the null terminated string for a substring. Behavior is undefined if `str`
    /// or `search` are nullptr.
    ///
    /// \param str The string to search within.
    /// \param search The string to search for.
    /// \return A pointer to the start of the found substring in `str`, or nullptr if
    /// not found. If `str` is empty, then `search` is returned.
    [[nodiscard]] const char* StrFind(const char* str, const char* search);

    /// Searches up to `len` characters from the string for a character. It is assumed that
    /// `str` is at least `len` characters long. That is, null characters are not treated
    /// as the end of the string. Behavior is undefined if `str` is nullptr.
    ///
    /// \param str The string to search within.
    /// \param len The maximum number of characters to check in `str`
    /// \param search The character to search for.
    /// \return A pointer to the found character in `str`, or nullptr if not found.
    [[nodiscard]] const char* StrFindN(const char* str, uint32_t len, char search);

    /// Searches up to `len` characters from the string for a substring. It is assumed that
    /// `str` is at least `len` characters long. That is, null characters are not treated
    /// as the end of the string. Behavior is undefined if `str` or `search` are nullptr.
    ///
    /// \param str The string to search within.
    /// \param len The maximum number of characters to check in `str`
    /// \param search The string to search for.
    /// \return A pointer to the start of the found substring in `str`, or nullptr if
    /// not found. If `str` is empty, then `search` is returned.
    [[nodiscard]] const char* StrFindN(const char* str, uint32_t len, const char* search);

    /// Parses the string into a integral value and sets the pointer pointed to by `end` to point
    /// to the character past the last character interpreted. If `end` is a null pointer, it is
    /// ignored. Any leading whitespace characters are discarded until the first non-whitespace
    /// character is found.
    ///
    /// If successful, an integer value corresponding to the contents of `str` is returned.
    /// If the converted value falls out of range of corresponding return type, the value is
    /// clamped to the limits. That is, `Limits<T>::Min` or `Limits<T>::Max` is returned.
    /// If no conversion can be performed, zero is returned.
    ///
    /// \param[in] str The string to parse.
    /// \param[in,out] end Optional. A pointer to a pointer to the end of the string. If nullptr
    ///     (default) then string is parsed until a null terminator is reached. If not nullptr,
    ///     then the pointer pointed to by `end` is set to point to the character past the last
    ///     character interpreted in `str`.
    /// \param[in] base Optional. The numerical base of the value being parsed. The set of valid
    ///     values for base is `{0,2,3,...,36}`. If base is zero, the base is determined by the
    ///     format in the sequence: a leading `0x` or `0X` means base 16, a leading `0` means base
    ///     8, and anything else means base 10. The default value is base 10.
    /// \return The parsed number.
    template <typename T>
    [[nodiscard]] T StrToInt(const char* str, const char** end = nullptr, int32_t base = 10);

    /// Parses the string into a floating point value and sets the pointer pointed to by `end` to
    /// point to the character past the last character interpreted. If `end` is a null pointer, it
    /// is ignored. Any leading whitespace characters are discarded until the first non-whitespace
    /// character is found.
    ///
    /// If successful, a floating point value corresponding to the contents of str is returned.
    /// If the converted value falls out of range of corresponding return type, the value is
    /// clamped to the limits. That is, `Limits<T>::Min` or `Limits<T>::Max` is returned.
    /// If no conversion can be performed, zero is returned.
    ///
    /// \param[in] str The string to parse.
    /// \param[in,out] end Optional. A pointer to a pointer to the end of the string. If nullptr
    ///     (default) then string is parsed until a null terminator is reached. If not nullptr,
    ///     then the pointer pointed to by `end` is set to point to the character past the last
    ///     character interpreted in `str`.
    /// \return The parsed number.
    template <typename T = float>
    [[nodiscard]] T StrToFloat(const char* str, const char** end = nullptr);
}

#include "he/core/inline/string_ops.inl"
