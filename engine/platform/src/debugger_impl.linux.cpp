// Copyright Chad Engler

#include "debugger_impl.h"

#if defined(HE_PLATFORM_LINUX)

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

namespace he
{
    void DebuggerImpl::Print(const char* s) const
    {
        fputs(s, stderr);
    }

    bool DebuggerImpl::IsAttached() const
    {
        return false;
    }
}

#endif
