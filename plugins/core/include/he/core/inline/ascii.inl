// Copyright Chad Engler

namespace he
{
    template <bool (*fn)(char c)>
    constexpr bool _IsCharTest(const char* str)
    {
        while (*str)
        {
            if (!fn(*str))
                return false;

            ++str;
        }
        return true;
    }

    constexpr bool IsWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
    }

    constexpr bool IsWhitespace(const char* str)
    {
        return _IsCharTest<IsWhitespace>(str);
    }

    constexpr bool IsUpper(char c)
    {
        return c >= 'A' && c <= 'Z';
    }

    constexpr bool IsUpper(const char* str)
    {
        return _IsCharTest<IsUpper>(str);
    }

    constexpr bool IsLower(char c)
    {
        return c >= 'a' && c <= 'z';
    }

    constexpr bool IsLower(const char* str)
    {
        return _IsCharTest<IsLower>(str);
    }

    constexpr bool IsAlpha(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    constexpr bool IsAlpha(const char* str)
    {
        return _IsCharTest<IsAlpha>(str);
    }

    constexpr bool IsNumeric(char c)
    {
        return c >= '0' && c <= '9';
    }

    constexpr bool IsNumeric(const char* str)
    {
        return _IsCharTest<IsNumeric>(str);
    }

    constexpr bool IsAlphaNum(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }

    constexpr bool IsAlphaNum(const char* str)
    {
        return _IsCharTest<IsAlphaNum>(str);
    }

    constexpr bool IsInteger(const char* str)
    {
        if (*str == '-')
            str++;

        return IsNumeric(str);
    }

    constexpr bool IsFloat(const char* str)
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

    constexpr bool IsHex(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    constexpr bool IsHex(const char* str)
    {
        return _IsCharTest<IsHex>(str);
    }

    constexpr bool IsPrint(char c)
    {
        return c >= ' ' && c <= '~';
    }

    constexpr bool IsPrint(const char* str)
    {
        return _IsCharTest<IsPrint>(str);
    }

    constexpr char ToUpper(char c)
    {
        return IsLower(c) ? c + 'A' - 'a' : c;
    }

    constexpr void ToUpper(char* str)
    {
        while (*str)
        {
            *str = ToUpper(*str);
            ++str;
        }
    }

    constexpr char ToLower(char c)
    {
        return IsUpper(c) ? c + 'a' - 'A' : c;
    }

    constexpr void ToLower(char* str)
    {
        while (*str)
        {
            *str = ToLower(*str);
            ++str;
        }
    }

    constexpr char ToHex(uint8_t nibble, bool upperCase)
    {
        constexpr char HexDigits[] = "0123456789abcdef";

        if (nibble < (sizeof(HexDigits) - 1))
        {
            char digit = HexDigits[nibble];
            return upperCase ? ToUpper(digit) : digit;
        }

        return '\0';
    }

    constexpr uint8_t HexToNibble(char c)
    {
        return (c >= '0' && c <= '9') ? c - '0'
            : c >= 'a' && c <= 'f' ? 10 + c - 'a'
            : c >= 'A' && c <= 'F' ? 10 + c - 'A'
            : 0;
    }

    constexpr uint8_t HexPairToByte(char a, char b)
    {
        return HexToNibble(a) * 16 + HexToNibble(b);
    }
}
