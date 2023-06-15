// Copyright Chad Engler

#include "he/core/string_ops.h"

#include "he/core/concepts.h"
#include "he/core/limits.h"
#include "he/core/type_traits.h"

#include <cstdlib>
#include <cstring>

namespace he
{
    uint32_t StrLen(const char* s)
    {
        return static_cast<uint32_t>(strlen(s));
    }

    uint32_t StrLenN(const char* s, uint32_t len)
    {
        return static_cast<uint32_t>(strnlen(s, len));
    }

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
        return strcasecmp(a, b);
    #endif
    }

    int32_t StrCompNI(const char* a, const char* b, uint32_t len)
    {
    #if HE_COMPILER_MSVC
        return _strnicmp(a, b, len);
    #else
        return strncasecmp(a, b, len);
    #endif
    }

    char* StrDup(const char* src, Allocator& allocator)
    {
        uint32_t len = StrLen(src);
        return StrDupN(src, len, allocator);
    }

    char* StrDupN(const char* src, uint32_t len, Allocator& allocator)
    {
        char* dst = allocator.Malloc<char>(len + 1);
        MemCopy(dst, src, len);
        dst[len] = '\0';
        return dst;
    }

    uint32_t StrCopy(char* dst, uint32_t dstLen, const char* src)
    {
        if (dstLen == 0)
            return 0;

        --dstLen;

        const uint32_t srcLen = StrLen(src);
        const uint32_t len = srcLen < dstLen ? srcLen : dstLen;

        MemCopy(dst, src, len);
        dst[len] = '\0';

        return len;
    }

    uint32_t StrCopyN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
    {
        if (dstLen == 0)
            return 0;

        --dstLen;

        srcLen = StrLenN(src, srcLen);
        const uint32_t len = srcLen < dstLen ? srcLen : dstLen;

        MemCopy(dst, src, len);
        dst[len] = '\0';

        return len;
    }

    uint32_t StrCat(char* dst, uint32_t dstLen, const char* src)
    {
        uint32_t n = StrLen(dst);
        dstLen = dstLen > n ? dstLen - n : 0;
        return n + StrCopy(dst + n, dstLen, src);
    }

    uint32_t StrCatN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
    {
        uint32_t n = StrLen(dst);
        dstLen = dstLen > n ? dstLen - n : 0;
        return n + StrCopyN(dst + n, dstLen, src, srcLen);
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

#if HE_COMPILER_MSVC
    #define HE_STRTOLL _strtoi64
    #define HE_STRTOULL _strtoui64
#else
    #define HE_STRTOLL std::strtoll
    #define HE_STRTOULL std::strtoull
#endif

    template <AnyOf<signed char, char, short, int, long, long long> T>
    static T StrToIntImpl(const char* str, const char** end, int32_t base)
    {
        static_assert(IsSigned<char>, "This function assumes char is signed.");
        const long long val = HE_STRTOLL(str, const_cast<char**>(end), base);
        if (val == 0)
            return 0;

        if constexpr (sizeof(T) < 8)
        {
            if (val < Limits<T>::Min)
                return Limits<T>::Min;

            if (val > Limits<T>::Max)
                return Limits<T>::Max;
        }

        return static_cast<T>(val);
    }

    template <AnyOf<unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long> T>
    static T StrToIntImpl(const char* str, const char** end, int32_t base)
    {
        const unsigned long long val = HE_STRTOULL(str, const_cast<char**>(end), base);
        if (val == 0)
            return 0;

        if constexpr (sizeof(T) < 8)
        {
            if (val > Limits<T>::Max)
                return Limits<T>::Max;
        }

        return static_cast<T>(val);
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

    template <>
    float StrToFloat<float>(const char* str, const char** end)
    {
        return std::strtof(str, const_cast<char**>(end));
    }

    template <>
    double StrToFloat<double>(const char* str, const char** end)
    {
        return std::strtod(str, const_cast<char**>(end));
    }
}
