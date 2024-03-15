// Copyright Chad Engler

#include "he/core/string_ops.h"

#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/limits.h"
#include "he/core/memory_ops.h"
#include "he/core/type_traits.h"

HE_PUSH_WARNINGS();
HE_DISABLE_MSVC_WARNING(4702); // unreachable code
#include "fast_float/fast_float.h"
HE_POP_WARNINGS();

#include <stdlib.h>
#include <string.h>

namespace he
{
    int32_t StrComp(const char* a, const char* b)
    {
        return strcmp(a, b);
    }

    int32_t StrCompN(const char* a, const char* b, uint32_t len)
    {
        return strncmp(a, b, len);
    }

    int32_t StrCompI(const char* a, const char* b)
    {
    #if HE_COMPILER_MSVC
        return _stricmp(a, b);
    #else
        return __builtin_strcasecmp(a, b);
    #endif
    }

    int32_t StrCompNI(const char* a, const char* b, uint32_t len)
    {
    #if HE_COMPILER_MSVC
        return _strnicmp(a, b, len);
    #else
        return __builtin_strncasecmp(a, b, len);
    #endif
    }

    const char* StrFind(const char* str, char search)
    {
        return strchr(str, search);
    }

    const char* StrFind(const char* str, const char* search)
    {
        return strstr(str, search);
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

    template <typename T>
    HE_FORCE_INLINE T StrToIntImpl(const char* str, const char** end, int32_t base)
    {
        const char* strEnd = nullptr;
        if (end)
            strEnd = *end;
        else
            strEnd = str + StrLen(str);

        T value = 0;
        const fast_float::from_chars_result result = fast_float::from_chars(str, strEnd, value, base);

        if (end)
            *end = result.ptr;

        return value;
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
