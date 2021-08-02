// Copyright Chad Engler

#include "he/core/wstr.h"

#include "he/core/assert.h"

#include <cwchar>

namespace he
{
#if !defined(HE_PLATFORM_API_WIN32)
    uint32_t MBToWCStr(wchar_t* dst, uint32_t dstLen, const char* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        std::mbstate_t state{};
        size_t result = std::mbsrtowcs(dst, &src, dst ? dstLen : 0, &state);
        return static_cast<uint32_t>(result) + 1; // +1 for null
    }

    uint32_t WCToMBStr(char* dst, uint32_t dstLen, const wchar_t* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        std::mbstate_t state{};
        size_t result = std::mbsrtowcs(dst, &src, dst ? dstLen : 0, &state);
        return static_cast<uint32_t>(result) + 1; // +1 for null
    }
#endif

    int32_t WCStrCmp(const wchar_t* a, const wchar_t* b)
    {
        return wcscmp(a, b);
    }
}
