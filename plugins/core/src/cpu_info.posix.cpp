// Copyright Chad Engler

#include "he/core/cpu_info.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <unistd.h>

namespace he
{
    void _FillPlatformCpuInfo(CpuInfo& info)
    {
    #if !defined(_SC_NPROCESSORS_ONLN)
        #error "Posix CPU info relies on _SC_NPROCESSORS_ONLN"
    #endif

        info.coreCount = sysconf(_SC_NPROCESSORS_ONLN);
        info.threadCount = info.coreCount;
    }
}

#endif
