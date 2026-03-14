// Copyright Chad Engler

#include "he/core/types.h"

#include "he/core/inline/ascii_table.inl"

namespace he
{
    template <bool (*fn)(char c)>
    constexpr bool _IsCharTest(const char* str) noexcept
    {
        while (*str)
        {
            if (!fn(*str))
                return false;

            ++str;
        }
        return true;
    }

    constexpr bool IsWhitespace(char c) noexcept
    {
        const _AsciiCategory category = _AsciiCategoryLookup[static_cast<uint8_t>(c)];
        return category == _AsciiCategory::Whitespace;
    }

    constexpr bool IsWhitespace(const char* str) noexcept
    {
        return _IsCharTest<IsWhitespace>(str);
    }

    constexpr bool IsUpper(char c) noexcept
    {
        const _AsciiCategory category = _AsciiCategoryLookup[static_cast<uint8_t>(c)];
        return category == _AsciiCategory::UpperAlpha;
    }

    constexpr bool IsUpper(const char* str) noexcept
    {
        return _IsCharTest<IsUpper>(str);
    }

    constexpr bool IsLower(char c) noexcept
    {
        const _AsciiCategory category = _AsciiCategoryLookup[static_cast<uint8_t>(c)];
        return category == _AsciiCategory::LowerAlpha;
    }

    constexpr bool IsLower(const char* str) noexcept
    {
        return _IsCharTest<IsLower>(str);
    }

    constexpr bool IsAlpha(char c) noexcept
    {
        const _AsciiCategory category = _AsciiCategoryLookup[static_cast<uint8_t>(c)];
        return category == _AsciiCategory::LowerAlpha || category == _AsciiCategory::UpperAlpha;
    }

    constexpr bool IsAlpha(const char* str) noexcept
    {
        return _IsCharTest<IsAlpha>(str);
    }

    constexpr bool IsNumeric(char c) noexcept
    {
        const _AsciiCategory category = _AsciiCategoryLookup[static_cast<uint8_t>(c)];
        return category == _AsciiCategory::Numeric;
    }

    constexpr bool IsNumeric(const char* str) noexcept
    {
        return _IsCharTest<IsNumeric>(str);
    }

    constexpr bool IsAlphaNum(char c) noexcept
    {
        const _AsciiCategory category = _AsciiCategoryLookup[static_cast<uint8_t>(c)];
        return category == _AsciiCategory::LowerAlpha || category == _AsciiCategory::UpperAlpha || category == _AsciiCategory::Numeric;
    }

    constexpr bool IsAlphaNum(const char* str) noexcept
    {
        return _IsCharTest<IsAlphaNum>(str);
    }

    constexpr bool IsInteger(const char* str) noexcept
    {
        if (*str == '-')
            str++;

        return IsNumeric(str);
    }

    constexpr bool IsFloat(const char* str) noexcept
    {
        if (*str == '-')
            str++;

        bool seenPeriod = false;

        while (*str)
        {
            if (!IsNumeric(*str))
            {
                if (*str == '.')
                {
                    if (seenPeriod)
                        return false;
                    seenPeriod = true;
                }
                else
                {
                    return false;
                }
            }

            ++str;
        }
        return true;
    }

    constexpr bool IsHex(char c) noexcept
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    constexpr bool IsHex(const char* str) noexcept
    {
        return _IsCharTest<IsHex>(str);
    }

    constexpr bool IsPrint(char c) noexcept
    {
        const uint8_t value = static_cast<uint8_t>(c);
        return value >= static_cast<uint8_t>(' ') && value <= static_cast<uint8_t>('~');
    }

    constexpr bool IsPrint(const char* str) noexcept
    {
        return _IsCharTest<IsPrint>(str);
    }

    constexpr char ToUpper(char c) noexcept
    {
        return IsLower(c) ? c + 'A' - 'a' : c;
    }

    constexpr void ToUpper(char* str) noexcept
    {
        while (*str)
        {
            *str = ToUpper(*str);
            ++str;
        }
    }

    constexpr char ToLower(char c) noexcept
    {
        return IsUpper(c) ? c + 'a' - 'A' : c;
    }

    constexpr void ToLower(char* str) noexcept
    {
        while (*str)
        {
            *str = ToLower(*str);
            ++str;
        }
    }

    constexpr char ToHex(uint8_t nibble, bool upperCase) noexcept
    {
        constexpr char HexDigits[] = "0123456789abcdef";

        if (nibble < (sizeof(HexDigits) - 1))
        {
            char digit = HexDigits[nibble];
            return upperCase ? ToUpper(digit) : digit;
        }

        return '\0';
    }

    constexpr uint8_t HexToNibble(char c) noexcept
    {
        return (c >= '0' && c <= '9') ? c - '0'
            : c >= 'a' && c <= 'f' ? 10 + c - 'a'
            : c >= 'A' && c <= 'F' ? 10 + c - 'A'
            : 0;
    }

    constexpr uint8_t HexPairToByte(char a, char b) noexcept
    {
        return HexToNibble(a) * 16 + HexToNibble(b);
    }
}
