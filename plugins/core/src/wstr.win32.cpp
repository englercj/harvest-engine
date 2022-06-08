// Copyright Chad Engler

#include "he/core/wstr.h"

#include "he/core/assert.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    uint32_t MBToWCStr(wchar_t* dst, uint32_t dstLen, const char* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        const int32_t result = ::MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, static_cast<int32_t>(dstLen));
        return static_cast<uint32_t>(result);
    }

    uint32_t WCToMBStr(char* dst, uint32_t dstLen, const wchar_t* src)
    {
        HE_ASSERT(src);
        HE_ASSERT((dst && dstLen) || (!dst && !dstLen));

        const int32_t result = ::WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, static_cast<int32_t>(dstLen), nullptr, nullptr);
        return static_cast<uint32_t>(result);
    }

    void WCToMBStr(String& dst, const wchar_t* src)
    {
        const int32_t requiredLen = ::WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);

        if (requiredLen <= 0)
        {
            dst.Clear();
            return;
        }

        dst.Resize(requiredLen, DefaultInit);

        const int32_t len = ::WideCharToMultiByte(CP_UTF8, 0, src, -1, dst.Data(), dst.Size(), nullptr, nullptr);

        if (len > 0)
            dst.Resize(len - 1);
        else
            dst.Clear();
    }

    void WCToMBStr(String& dst, const wchar_t* src, uint32_t srcLen_)
    {
        const int32_t srcLen = static_cast<int32_t>(srcLen_);
        const int32_t requiredLen = ::WideCharToMultiByte(CP_UTF8, 0, src, srcLen, nullptr, 0, nullptr, nullptr);

        if (requiredLen <= 0)
        {
            dst.Clear();
            return;
        }

        dst.Resize(requiredLen, DefaultInit);

        const int32_t len = ::WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dst.Data(), dst.Size(), nullptr, nullptr);

        if (len > 0)
            dst.Resize(len - 1);
        else
            dst.Clear();
    }
}

#endif
