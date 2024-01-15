// Copyright Chad Engler

#include "he/core/cpu_info.h"

#include "he/core/utils.h"

#if defined(HE_PLATFORM_WASM)

#include "wasm_core.js.h"

namespace he
{
    void _FillPlatformCpuInfo(CpuInfo& info)
    {
        info.coreCount = 1;
        info.threadCount = heWASM_GetHardwareConcurrency();
        info.cacheLineSize = 64;
    }
}

#endif
