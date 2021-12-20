// Copyright Chad Engler

#include "he/core/thread.h"

#if defined(HE_PLATFORM_API_POSIX)

namespace he
{
    void SetCurrentThreadName(const char* name)
    {
        prctl(PR_SET_NAME, name, 0, 0, 0);
    }
}

#endif
