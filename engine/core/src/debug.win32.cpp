#include "he/core/debug.h"

#include "he/core/platform.h"
#include "he/core/string.h"
#include "he/core/wstr.h"

#if HE_API_WIN32

#include "win32_min.h"

namespace he
{
    void OutputToDebugger(const char* s)
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
