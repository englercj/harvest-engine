// Copyright Chad Engler

#include "he/core/cpu_info.h"

#include "he/core/utils.h"

#if defined(HE_PLATFORM_LINUX)

#include <sched.h>
#include <stdio.h>

namespace he
{
    void _FillPlatformCpuInfo(CpuInfo& info)
    {
        cpu_set_t cpusMask;
        CPU_ZERO(&cpusMask);

        if (sched_getaffinity(0, sizeof(cpusMask), &cpusMask) == 0)
        {
            info.coreCount = CPU_COUNT(&cpusMask);
            info.threadCount = CPU_COUNT(&cpusMask);
        }

        info.coreCount = Max(info.coreCount, 1u);
        info.threadCount = Max(info.threadCount, 1u);

        info.cacheLineSize = 64;
        FILE* sysFile = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
        if (sysFile)
        {
            int lineSize = 0;
            if (fscanf(sysFile, "%d", &lineSize) == 1)
            {
                if (lineSize > 0)
                {
                    info.cacheLineSize = static_cast<uint32_t>(lineSize);
                }
            }
            fclose(sysFile);
        }
    }
}

#endif
