// Copyright Chad Engler

#include "he/core/debug.h"

#if defined(HE_PLATFORM_LINUX)

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

namespace he
{
    void PrintToDebugger(const char* s) const
    {
        fputs(s, stderr);
    }

    bool IsDebuggerAttached() const
    {
        return false;
    }
}

#endif
