// Copyright Chad Engler

#include "he/core/cpu_info.h"

#include "he/core/utils.h"

#if defined(HE_PLATFORM_WASM)

#include "wasm_core.js.h"

namespace he
{
    void _FillPlatformCpuInfo(CpuInfo& info)
    {
        const uint32_t threads = heWASM_GetHardwareConcurrency();

        // We have no way of detecting the physical core count, so we just assume it's half the
        // thread count, since a common configuration is each physical core has two logical cores.
        info.coreCount = Max(1u, threads / 2);
        info.threadCount = threads;

        // We can't know what this is for real but 64 is a very common and safe value to use.
        info.cacheLineSize = 64;
    }
}

#endif
