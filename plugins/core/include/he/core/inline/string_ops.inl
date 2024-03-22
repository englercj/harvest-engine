// Copyright Chad Engler

#include "he/core/allocator.h"
#include "he/core/memory_ops.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    constexpr bool StrEmpty(const char* s)
    {
        return !s || *s == '\0';
    }

    constexpr uint32_t StrLen(const char* s)
    {
    #if HE_COMPILER_MSVC || !HE_HAS_LIBC
        // As of C++17 MSVC also has `__builtin_strlen`, but it is not constexpr and in my tests
        // this loop performed the same in Release and much better in Debug. The opposite was
        // true for clang/gcc where `__builtin_strlen` was far faster than the loop in all cases.
        const char* p = s;
        while (*p != '\0')
            ++p;
        return static_cast<uint32_t>(p - s);
    #else
        return static_cast<uint32_t>(__builtin_strlen(s));
    #endif
    }

    inline uint32_t StrLenN(const char* s, uint32_t len)
    {
        const char* p = static_cast<const char*>(MemChr(s, 0, len));
        return p ? static_cast<uint32_t>(p - s) : len;
    }

    inline char* StrDup(const char* src, Allocator& allocator)
    {
        const uint32_t len = StrLen(src);
        return StrDupN(src, len, allocator);
    }

    inline char* StrDupN(const char* src, uint32_t len, Allocator& allocator)
    {
        char* dst = allocator.Malloc<char>(len + 1);
        MemCopy(dst, src, len);
        dst[len] = '\0';
        return dst;
    }

    inline bool StrEqual(const char* a, const char* b)
    {
        return StrComp(a, b) == 0;
    }

    inline bool StrEqualN(const char* a, const char* b, uint32_t len)
    {
        return StrCompN(a, b, len) == 0;
    }

    inline bool StrEqualI(const char* a, const char* b)
    {
        return StrCompI(a, b) == 0;
    }

    inline bool StrEqualNI(const char* a, const char* b, uint32_t len)
    {
        return StrCompNI(a, b, len) == 0;
    }

    inline bool StrLess(const char* a, const char* b)
    {
        return StrComp(a, b) < 0;
    }

    inline bool StrLessN(const char* a, const char* b, uint32_t len)
    {
        return StrCompN(a, b, len) < 0;
    }

    inline uint32_t StrCopy(char* dst, uint32_t dstLen, const char* src)
    {
        if (dstLen == 0)
            return 0;

        // subtract one for the space needed for the null terminator
        --dstLen;

        const uint32_t srcLen = StrLen(src);
        const uint32_t len = srcLen < dstLen ? srcLen : dstLen;

        MemCopy(dst, src, len);
        dst[len] = '\0';

        return len;
    }

    template <uint32_t N>
    inline uint32_t StrCopy(char (&dst)[N], const char* src)
    {
        return StrCopy(dst, N, src);
    }

    inline uint32_t StrCopyN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
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

    template <uint32_t N>
    inline uint32_t StrCopyN(char (&dst)[N], const char* src, uint32_t srcLen)
    {
        return StrCopyN(dst, N, src, srcLen);
    }

    inline uint32_t StrCat(char* dst, uint32_t dstLen, const char* src)
    {
        uint32_t n = StrLen(dst);
        dstLen = dstLen > n ? dstLen - n : 0;
        return n + StrCopy(dst + n, dstLen, src);
    }

    template <uint32_t N>
    inline uint32_t StrCat(char (&dst)[N], const char* src)
    {
        return StrCat(dst, N, src);
    }

    inline uint32_t StrCatN(char* dst, uint32_t dstLen, const char* src, uint32_t srcLen)
    {
        uint32_t n = StrLen(dst);
        dstLen = dstLen > n ? dstLen - n : 0;
        return n + StrCopyN(dst + n, dstLen, src, srcLen);
    }

    template <uint32_t N>
    inline uint32_t StrCatN(char (&dst)[N], const char* src, uint32_t srcLen)
    {
        return StrCatN(dst, N, src, srcLen);
    }
}
