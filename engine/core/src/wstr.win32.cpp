// Copyright Chad Engler

#include "he/core/wstr.h"

#include "he/core/assert.h"
#include "he/core/platform.h"

#if HE_API_WIN32

#include "win32_min.h"

namespace he
{
    uint32_t MBToWCStr(wchar_t* dst, uint32_t dstLen, const char* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        int32_t result = MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, static_cast<int32_t>(dstLen));
        return static_cast<uint32_t>(result);
    }

    uint32_t WCToMBStr(char* dst, uint32_t dstLen, const wchar_t* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        int32_t result = WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, static_cast<int32_t>(dstLen), nullptr, nullptr);
        return static_cast<uint32_t>(result);
    }
}

#endif
