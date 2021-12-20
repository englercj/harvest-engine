// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    void SetCurrentThreadName(const char* name)
    {
        ::SetThreadDescription(::GetCurrentThread(), HE_TO_WSTR(name));
    }
}

#endif
