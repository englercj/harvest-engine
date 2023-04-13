// Copyright Chad Engler

#include "he/core/cpu_info.h"

#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_POSIX) && !defined(HE_PLATFORM_LINUX)

#include <unistd.h>

namespace he
{
    void _FillPlatformCpuInfo(CpuInfo& info)
    {
        info.coreCount = Max(1u, sysconf(_SC_NPROCESSORS_ONLN));
        info.threadCount = info.coreCount;
        info.cacheLineSize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    }
}

#endif
