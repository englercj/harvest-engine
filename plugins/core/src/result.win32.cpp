// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/fmt.h"
#include "he/core/string.h"
#include "he/core/wstr.h"

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

    void Result::ToString(String& out) const
    {
        const DWORD code = HRESULT_FACILITY(m_code) == FACILITY_WIN32 ? HRESULT_CODE(m_code) : m_code;

        wchar_t src[4096];
        DWORD srcLen = ::FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            HRESULT_CODE(code),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            src,
            HE_LENGTH_OF(src),
            nullptr);

        if (srcLen == 0)
        {
            FormatTo(out, "Unknown error: {0:#010x} ({0})", m_code);
            return;
        }

        // Remove a trailing period & \r\n for consistency with posix messages.
        if (srcLen >= 3 && src[srcLen - 3] == '.')
            src[srcLen -= 3] = 0;

        WCToMBStr(out, src);
    }
}

#endif
