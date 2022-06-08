// Copyright Chad Engler

#include "he/core/wstr.h"

#include "he/core/assert.h"

#include <cwchar>
#include <string>

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
        size_t result = std::wcsrtombs(dst, &src, dst ? dstLen : 0, &state);
        return static_cast<uint32_t>(result) + 1; // +1 for null
    }

    void WCToMBStr(String& dst, const wchar_t* src)
    {
        HE_ASSERT(src);

        std::mbstate_t state{};
        const size_t requiredLen = std::wcsrtombs(nullptr, &src, 0, &state);

        if (requiredLen == 0 || requiredLen == static_cast<size_t>(-1))
        {
            dst.Clear();
            return;
        }

        dst.Resize(static_cast<uint32_t>(requiredLen), DefaultInit);

        const size_t len = std::wcsrtombs(dst.Data(), &src, dst.Size(), &state);

        if (len > 0)
            dst.Resize(len);
        else
            dst.Clear();
    }

    void WCToMBStr(String& dst, const wchar_t* src, uint32_t srcLen)
    {
        HE_ASSERT(src);
        std::wstring wsrc(src, srcLen);
        WCToMBStr(dst, wsrc.c_str());
    }
#endif

    int32_t WCStrCmp(const wchar_t* a, const wchar_t* b)
    {
        return wcscmp(a, b);
    }
}
