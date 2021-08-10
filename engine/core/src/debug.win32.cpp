// Copyright Chad Engler

#include "debugger_impl.h"

#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    void DebuggerImpl::Print(const char* s) const
    {
        wchar_t* wideStr = HE_TO_WSTR(s);
        OutputDebugStringW(wideStr);
    }

    bool DebuggerImpl::IsAttached() const
    {
        return !!IsDebuggerPresent();
    }
}

#endif
