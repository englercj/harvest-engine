// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he
{
    /// Checks if the character is whitespace (space, tab, newline, etc)
    ///
    /// \param c The character to check.
    /// \return True if the character is a whitespace character.
    constexpr bool IsWhitespace(char c);

    /// Checks if the null terminated string contains only whitespace characters.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only whitespace characters.
    constexpr bool IsWhitespace(const char* str);

    /// Checks if the character is uppercase.
    ///
    /// \param c The character to check.
    /// \return True if the character is in the range `[A,Z]`.
    constexpr bool IsUpper(char c);

    /// Checks if the null terminated string contains only uppercase characters.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[A,Z]`.
    constexpr bool IsUpper(const char* str);

    /// Checks if the character is lowercase.
    ///
    /// \param c The character to check.
    /// \return True if the character is in the range `[a,z]`.
    constexpr bool IsLower(char c);

    /// Checks if the null terminated string contains only lowercase characters.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[a,z]`.
    constexpr bool IsLower(const char* str);

    /// Checks if the character is an alphabet character.
    ///
    /// \param c The character to check.
    /// \return True if the character is in the range `[a,z]` or `[A,Z]`.
    constexpr bool IsAlpha(char c);

    /// Checks if the null terminated string contains only alphabet characters.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[a,z]` or `[A,Z]`.
    constexpr bool IsAlpha(const char* str);

    /// Checks if the character is a numeric digit.
    ///
    /// \param c The character to check.
    /// \return True if the character is in the range `[0,9]`.
    constexpr bool IsNumeric(char c);

    /// Checks if the null terminated string contains only numeric digits.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[0,9]`.
    constexpr bool IsNumeric(const char* str);

    /// Checks if the character is an alphabet character or numeric digit.
    ///
    /// \param c The character to check.
    /// \return True if the character is in the range `[a,z]`, `[A,Z]`, or `[0,9]`.
    constexpr bool IsAlphaNum(char c);

    /// Checks if the null terminated string contains only alphabet characters or numeric digits.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[a,z]`, `[A,Z]`, or `[0,9]`.
    constexpr bool IsAlphaNum(const char* str);

    /// Checks if the null terminated string contains only numeric digits, and optional leading dash.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[0,9]`, and an optional leading `-`.
    constexpr bool IsIntegral(const char* str);

    /// Checks if the null terminated string contains only numeric digits, a single period, and optional leading dash.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[0,9]`, a single `.`, and an optional leading `-`.
    constexpr bool IsFloatingPoint(const char* str);

    /// Checks if the character is a hexadecimal digit.
    ///
    /// \param c The character to check.
    /// \return True if the character is in the range `[a,f]`, `[A,F]`, or `[0,9]`.
    constexpr bool IsHex(char c);

    /// Checks if the null terminated string contains only hexadecimal digits.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[a,f]`, `[A,F]`, or `[0,9]`.
    constexpr bool IsHex(const char* str);

    /// Checks if the character is printable.
    ///
    /// \param c The character to check.
    /// \return True if the character is in the range `[ ,~]`.
    constexpr bool IsPrint(char c);

    /// Checks if the null terminated string contains only printable characters.
    ///
    /// \param str The string to check.
    /// \return True if the string contains only characters in the range `[ ,~]`.
    constexpr bool IsPrint(const char* str);

    /// Transforms the character to uppercase.
    ///
    /// \param c The character to transform.
    /// \return The transformed character.
    constexpr char ToUpper(char c);

    /// Transforms the characters in the null terminated string to uppercase.
    ///
    /// \param str The string to transform.
    constexpr void ToUpper(char* str);

    /// Transforms the character to lowercase.
    ///
    /// \param c The character to check.
    /// \return The transformed character.
    constexpr char ToLower(char c);

    /// Transforms the characters in the null terminated string to lowercase.
    ///
    /// \param str The string to transform.
    constexpr void ToLower(char* str);

    /// Transforms `nibble` to the ascii hex digit that represents it.
    ///
    /// \param nibble The value to transform into a hex digit. Must be in the range `[0, 16)`.
    /// \param upperCase When true returns an uppercase hex digit, lowercase otherwise.
    /// \return The hex digit that represents the value, or the null character ('\0') on invalid input.
    constexpr char ToHex(uint8_t nibble, bool upperCase = false);

    /// Gets the value of the hex digit.
    ///
    /// \param c The hex digit to get the value for.
    /// \return The nibble this hex digit represents. Characters that are not valid hex will return zero.
    constexpr uint8_t HexToNibble(char c);

    /// Gets the value of a pair hex digits.
    ///
    /// \param a The first hex digit in the pair.
    /// \param b The second hex digit in the pair.
    /// \return The byte this pair of hex digits represents. Characters that are not valid hex will return zero.
    constexpr uint8_t HexPairToByte(char a, char b);
}

#include "he/core/inline/ascii.inl"
