// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/alloca.h"
#include "he/core/string.h"
#include "he/core/wstr.h"

#include "fmt/format.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <winerror.h>

namespace he
{
    Result Result::Success{ 0 };
    Result Result::InvalidParameter{ ERROR_INVALID_PARAMETER };
    Result Result::NotSupported{ ERROR_INVALID_FUNCTION };

    Result Result::FromLastError()
    {
        return Win32Result(::GetLastError());
    }

    String Result::ToString(Allocator& allocator) const
    {
        String dst(allocator);

        wchar_t* src = nullptr;
        DWORD srcLen = ::FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER  | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            DWORD(m_code),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&src,
            0,
            nullptr);

        if (srcLen == 0 || src == nullptr)
        {
            ::LocalFree(src);

            fmt::memory_buffer buf;
            fmt::format_to(fmt::appender(buf), "Unknown error: {}", m_code);
            dst.Assign(buf.data(), static_cast<uint32_t>(buf.size()));
            return dst;
        }

        // Remove a trailing period & \r\n for consistency with posix messages.
        if (srcLen >= 3 && src[srcLen - 3] == '.')
            src[srcLen -= 3] = 0;

        WCToMBStr(dst, src);

        ::LocalFree(src);
        return dst;
    }
}

#endif
