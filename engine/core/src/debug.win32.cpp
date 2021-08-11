// Copyright Chad Engler

#include "he/core/debug.h"

#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    void PrintToDebugger(const char* s)
    {
        wchar_t* wideStr = HE_TO_WSTR(s);
        OutputDebugStringW(wideStr);
    }

    bool IsDebuggerAttached()
    {
        return !!IsDebuggerPresent();
    }
}

#endif
