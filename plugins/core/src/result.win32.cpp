// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/alloca.h"
#include "he/core/appender.h"
#include "he/core/string.h"
#include "he/core/wstr.h"

#include "fmt/core.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <winerror.h>

namespace he
{
    Result Result::Success{ 0 };
    Result Result::InvalidParameter{ ERROR_INVALID_PARAMETER };
    Result Result::NotSupported{ ERROR_NOT_SUPPORTED };

    Result Result::FromLastError()
    {
        return Win32Result(::GetLastError());
    }

    String Result::ToString(Allocator& allocator) const
    {
        String dst(allocator);

        wchar_t src[4096];
        DWORD srcLen = ::FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            m_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            src,
            HE_LENGTH_OF(src),
            nullptr);

        if (srcLen == 0 || src == nullptr)
        {
            fmt::format_to(Appender(dst), "Unknown error: {}", m_code);
            return dst;
        }

        // Remove a trailing period & \r\n for consistency with posix messages.
        if (srcLen >= 3 && src[srcLen - 3] == '.')
            src[srcLen -= 3] = 0;

        WCToMBStr(dst, src);
        return dst;
    }
}

#endif
