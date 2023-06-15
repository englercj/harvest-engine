// Copyright Chad Engler

namespace he
{
    constexpr bool StrEmpty(const char* s)
    {
        return !s || *s == '\0';
    }

    constexpr uint32_t StrLenConst(const char* s)
    {
        // As of C++17 MSVC also has __builtin_strlen, but in my tests this loop performed the
        // same in Release and much better in Debug. The opposite was true for clang/gcc where
        // __builtin_strlen was far faster than the loop in all cases.
    #if HE_COMPILER_MSVC
        const char* p = s;
        while (*p != '\0')
            ++p;
        return static_cast<uint32_t>(p - s);
    #else
        return static_cast<uint32_t>(__builtin_strlen(s));
    #endif
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

    template <uint32_t N>
    uint32_t StrCopy(char (&dst)[N], const char* src)
    {
        return StrCopy(dst, N, src);
    }

    template <uint32_t N>
    uint32_t StrCopyN(char (&dst)[N], const char* src, uint32_t srcLen)
    {
        return StrCopyN(dst, N, src, srcLen);
    }

    template <uint32_t N>
    uint32_t StrCat(char (&dst)[N], const char* src)
    {
        return StrCat(dst, N, src);
    }

    template <uint32_t N>
    uint32_t StrCatN(char (&dst)[N], const char* src, uint32_t srcLen)
    {
        return StrCatN(dst, N, src, srcLen);
    }
}
