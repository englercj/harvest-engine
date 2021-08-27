// Copyright Chad Engler

namespace he
{
    constexpr bool String::IsEmpty(const char* s)
    {
        return !s || *s == '\0';
    }

    constexpr uint32_t String::LengthConst(const char* s)
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
}
