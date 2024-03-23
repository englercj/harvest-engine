// Copyright Chad Engler

#include "he/core/string_ops.h"

#include "he/core/ascii.h"
#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/limits.h"
#include "he/core/memory_ops.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"

HE_PUSH_WARNINGS();
HE_DISABLE_MSVC_WARNING(4702); // unreachable code
#define FASTFLOAT_SKIP_WHITE_SPACE 1
#define FASTFLOAT_ALLOWS_LEADING_PLUS 1
#include "fast_float/fast_float.h"
HE_POP_WARNINGS();

#if HE_HAS_LIBC
    #include <stdlib.h>
    #include <string.h>
#endif

namespace he
{
    int32_t StrComp(const char* a, const char* b)
    {
    #if HE_HAS_LIBC
        return strcmp(a, b);
    #else
        while (*a == *b && *a)
        {
            ++a;
            ++b;
        }
        return static_cast<int32_t>(*a) - static_cast<int32_t>(*b);
    #endif
    }

    int32_t StrCompN(const char* a, const char* b, uint32_t len)
    {
    #if HE_HAS_LIBC
        return strncmp(a, b, len);
    #else
        if (len-- == 0)
            return 0;

        while (*a && *b && len && *a == *b)
        {
            ++a;
            ++b;
            --len;
        }

        return static_cast<int32_t>(*a) - static_cast<int32_t>(*b);
    #endif
    }

    int32_t StrCompI(const char* a, const char* b)
    {
    #if HE_HAS_LIBC
    #if HE_COMPILER_MSVC
        return _stricmp(a, b);
    #else
        return __builtin_strcasecmp(a, b);
    #endif
    #else
        while (*a && *b && (*a == *b || ToLower(*a) == ToLower(*b)))
        {
            ++a;
            ++b;
        }
        return static_cast<int32_t>(ToLower(*a)) - static_cast<int32_t>(ToLower(*b));
    #endif
    }

    int32_t StrCompNI(const char* a, const char* b, uint32_t len)
    {
    #if HE_HAS_LIBC
    #if HE_COMPILER_MSVC
        return _strnicmp(a, b, len);
    #else
        return __builtin_strncasecmp(a, b, len);
    #endif
    #else
        if (len-- == 0)
            return 0;

        while (*a && *b && len && (*a == *b || ToLower(*a) == ToLower(*b)))
        {
            ++a;
            ++b;
            --len;
        }
        return static_cast<int32_t>(ToLower(*a)) - static_cast<int32_t>(ToLower(*b));
    #endif
    }

    const char* StrFind(const char* str, char search)
    {
    #if HE_HAS_LIBC
        return strchr(str, search);
    #else
        if (search == 0)
            return str + StrLen(str);

        constexpr size_t Alignment = sizeof(size_t);
        while (!IsAligned(str, Alignment))
        {
            if (*str == 0 || *str == search)
                return str;

            ++str;
        }

        constexpr size_t Ones = static_cast<size_t>(-1) / 0xff;
        constexpr size_t Highs = Ones * ((0xff / 2) + 1);

        #define HE_STRFIND_HAS_ZERO(x) (((x) - Ones) & (~(x) & Highs))

        const size_t* HE_MAY_ALIAS w = reinterpret_cast<const size_t* HE_MAY_ALIAS>(str);
        const size_t k = Ones * static_cast<unsigned char>(search);
        while (!HE_STRFIND_HAS_ZERO(*w) && !HE_STRFIND_HAS_ZERO(*w ^ k))
        {
            ++w;
        }
        str = reinterpret_cast<const char*>(w);

        #undef HE_MEMCHR_HAS_ZERO

        while (*str && *str != search)
        {
            ++str;
        }

        return *str == search ? str : 0;
    #endif
    }

    const char* StrFind(const char* str, const char* search)
    {
    #if HE_HAS_LIBC
        return strstr(str, search);
    #else
        if (!*search)
            return str;

        str = StrFind(str, *search);
        if (!str || !search[1])
            return str;

        if (!str[1])
            return nullptr;

        if (!search[2])
        {
            const uint16_t search16 = (search[0] << 8) | search[1];
            uint16_t str16 = (str[0] << 8) | str[1];
            ++str;
            while (*str && str16 != search16)
            {
                str16 = (str16 << 8) | *++str;
            }
            return *str ? str - 1 : nullptr;
        }

        if (!str[2])
            return nullptr;

        if (!search[3])
        {
            const uint32_t search24 = (static_cast<uint32_t>(search[0]) << 24) | (static_cast<uint32_t>(search[1]) << 16) | (static_cast<uint32_t>(search[2]) << 8);
            uint32_t str24 = (static_cast<uint32_t>(str[0]) << 24) | (static_cast<uint32_t>(str[1]) << 16) | (static_cast<uint32_t>(str[2]) << 8);
            str += 2;
            while (*str && str24 != search24)
            {
                str24 = (str24|*++str) << 8;
            }
            return *str ? str - 2 : 0;
        }

        if (!str[3])
            return nullptr;

        if (!search[4])
        {
            const uint32_t search32 = (static_cast<uint32_t>(search[0]) << 24) | (static_cast<uint32_t>(search[1]) << 16) | (static_cast<uint32_t>(search[2]) << 8) | static_cast<uint32_t>(search[3]);
            uint32_t str32 = (static_cast<uint32_t>(str[0]) << 24) | (static_cast<uint32_t>(str[1]) << 16) | (static_cast<uint32_t>(str[2]) << 8) | static_cast<uint32_t>(str[3]);
            str += 3;
            while (*str && str32 != search32)
            {
                str32 = (str32 << 8) | *++str;
            }
            return *str ? str - 2 : 0;
        }

        // TODO: optimize this
        do
        {
            if (StrEqual(str, search))
                return str;

            str = StrFind(++str, *search);
        } while (str);

        return nullptr;
    #endif
    }

    const char* StrFindN(const char* str, uint32_t len, char search)
    {
        return static_cast<const char*>(MemChr(str, search, len));
    }

    const char* StrFindN(const char* str, uint32_t len, const char* search)
    {
        const uint32_t searchLen = StrLenN(search, len);

        for (uint32_t i = 0; i <= (len - searchLen); ++i)
        {
            if (*str == *search && StrEqualN(str, search, searchLen))
                return str;

            ++str;
        }
        return nullptr;
    }

    const char* StrFindLast(const char* str, char search)
    {
        return StrFindLastN(str, StrLen(str), search);
    }

    // const char* StrFindLast(const char* str, const char* search)
    // {
    //     return StrFindLastN(str, StrLen(str), search);
    // }

    const char* StrFindLastN(const char* str, uint32_t len, char search)
    {
        while (len--)
        {
            if (str[len] == search)
                return str + len;
        }

        return nullptr;
    }

    // const char* StrFindLastN(const char* str, uint32_t len, const char* search)
    // {
    //     // TODO: implement
    // }

    template <typename T>
    HE_FORCE_INLINE T StrToIntImpl(const char* str, const char** end, int32_t base)
    {
        // A modified version of the `fast_float::from_chars` function.
        // Adds support for base prefixes (0x, 0b, 0o) and negative signs on unsigned types.

        const char* first = str;
        const char* strEnd = nullptr;
        if (end)
            strEnd = *end;
        else
            strEnd = str + StrLen(str);

        while (str < strEnd && IsWhitespace(*str))
            ++str;

        if (str == strEnd || base < 2 || base > 36)
        {
            if (end)
                *end = str;
            return 0;
        }

        const bool negative = *str == '-';
        if (negative || *str == '+')
            ++str;

        const char* numStart = str;

        while (str < strEnd && *str == '0')
            ++str;

        // Skip over supported prefixes for binary, octal, and hexadecimal bases.
        switch (base)
        {
            case 2: // '0b'
                if (str < strEnd && (*str == 'b' || *str == 'B'))
                    ++str;

                numStart = str;
                break;
            case 8: // '0o'
                if (str < strEnd && (*str == 'o' || *str == 'O'))
                    ++str;

                numStart = str;
                break;
            case 16: // '0b'
                if (str < strEnd && (*str == 'x' || *str == 'X'))
                    ++str;

                numStart = str;
                break;
        }

        while (str < strEnd && *str == '0')
            ++str;

        const bool hasLeadingZeros = str > numStart;
        const char* firstDigit = str;

        uint64_t i = 0;
        if (base == 10)
        {
            fast_float::loop_parse_if_eight_digits(str, strEnd, i);
        }

        while (str < strEnd)
        {
            uint8_t digit = fast_float::ch_to_digit(*str);
            if (digit >= base)
                break;
            i = (static_cast<uint64_t>(base) * i) + digit;
            ++str;
        }

        if (end)
            *end = str;

        const size_t digitCount = str - firstDigit;
        if (digitCount == 0)
        {
            if (hasLeadingZeros)
                return 0;

            // error: invalid input
            if (end)
                *end = first;
            return 0;
        }


        const size_t maxDigits = fast_float::max_digits_u64(base);
        if (digitCount > maxDigits)
        {
            // error: out of range
            return negative ? Limits<T>::Min : Limits<T>::Max;
        }

        if (digitCount == maxDigits && i < fast_float::min_safe_u64(base))
        {
            // error: out of range
            return negative ? Limits<T>::Min : Limits<T>::Max;
        }

        if (!IsSame<T, uint64_t>)
        {
            if (i > static_cast<uint64_t>(Limits<T>::Max) + static_cast<uint64_t>(negative))
            {
                // error: out of range
                return negative ? Limits<T>::Min : Limits<T>::Max;
            }
        }

        if (negative)
        {
            HE_PUSH_WARNINGS();
            HE_DISABLE_MSVC_WARNING(4146); // unary minus operator applied to unsigned type, result still unsigned
            // this weird workaround is required because:
            // - converting unsigned to signed when its value is greater than signed max is UB pre-C++23.
            // - reinterpret_casting (~i + 1) would work, but it is not constexpr
            // this is always optimized into a neg instruction (note: T is an integer type)
            return T(-Limits<T>::Max - T(i - static_cast<uint64_t>(Limits<T>::Max)));
            HE_POP_WARNINGS();
        }

        return static_cast<T>(i);
    }

    template <> signed char StrToInt<signed char>(const char* str, const char** end, int32_t base) { return StrToIntImpl<signed char>(str, end, base); }

    template <> char StrToInt<char>(const char* str, const char** end, int32_t base) { return StrToIntImpl<char>(str, end, base); }
    template <> short StrToInt<short>(const char* str, const char** end, int32_t base) { return StrToIntImpl<short>(str, end, base); }
    template <> int StrToInt<int>(const char* str, const char** end, int32_t base) { return StrToIntImpl<int>(str, end, base); }
    template <> long StrToInt<long>(const char* str, const char** end, int32_t base) { return StrToIntImpl<long>(str, end, base); }
    template <> long long StrToInt<long long>(const char* str, const char** end, int32_t base) { return StrToIntImpl<long long>(str, end, base); }

    template <> unsigned char StrToInt<unsigned char>(const char* str, const char** end, int32_t base) { return StrToIntImpl<unsigned char>(str, end, base); }
    template <> unsigned short StrToInt<unsigned short>(const char* str, const char** end, int32_t base) { return StrToIntImpl<unsigned short>(str, end, base); }
    template <> unsigned int StrToInt<unsigned int>(const char* str, const char** end, int32_t base) { return StrToIntImpl<unsigned int>(str, end, base); }
    template <> unsigned long StrToInt<unsigned long>(const char* str, const char** end, int32_t base) { return StrToIntImpl<unsigned long>(str, end, base); }
    template <> unsigned long long StrToInt<unsigned long long>(const char* str, const char** end, int32_t base) { return StrToIntImpl<unsigned long long>(str, end, base); }

    template <typename T>
    HE_FORCE_INLINE T StrToFloatImpl(const char* str, const char** end)
    {
        const char* strEnd = nullptr;
        if (end)
            strEnd = *end;
        else
            strEnd = str + StrLen(str);

        T value = 0;
        const fast_float::from_chars_result result = fast_float::from_chars(str, strEnd, value);

        if (end)
            *end = result.ptr;

        return value;
    }

    template <> float StrToFloat<float>(const char* str, const char** end) { return StrToFloatImpl<float>(str, end); }
    template <> double StrToFloat<double>(const char* str, const char** end) { return StrToFloatImpl<double>(str, end); }
}
