// Copyright Chad Engler

#include "he/core/debug.h"

#include "he/core/platform.h"
#include "he/core/string.h"

#if HE_PLATFORM_LINUX

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

namespace he
{
    void OutputToDebugger(const char* s)
    {
        fputs(s, stderr);
    }

    bool IsDebuggerAttached()
    {
        return false;
    }
}

#endif
