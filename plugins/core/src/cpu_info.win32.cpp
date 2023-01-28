// Copyright Chad Engler

#include "he/core/cpu_info.h"

#include "he/core/alloca.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

using Pfn_GetLogicalProcessorInformation = BOOL (WINAPI*)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

namespace he
{
    // Helper function to count set bits in the processor mask.
    static DWORD CountSetBits(ULONG_PTR bitMask)
    {
        DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
        DWORD bitSetCount = 0;
        ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
        DWORD i;

        for (i = 0; i <= LSHIFT; ++i)
        {
            bitSetCount += ((bitMask & bitTest) ? 1 : 0);
            bitTest /= 2;
        }

        return bitSetCount;
    }

    void _FillPlatformCpuInfo(CpuInfo& info)
    {
        Pfn_GetLogicalProcessorInformation glpi = reinterpret_cast<Pfn_GetLogicalProcessorInformation>(::GetProcAddress(GetModuleHandleW(L"kernel32"), "GetLogicalProcessorInformation"));

        DWORD bufferLen = 0;
        if (glpi != nullptr && glpi(nullptr, &bufferLen) == FALSE && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            const uint32_t count = bufferLen / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = HE_ALLOCA(SYSTEM_LOGICAL_PROCESSOR_INFORMATION, count);
            if (glpi(buffer, &bufferLen))
            {
                info.coreCount = 0;
                info.threadCount = 0;

                for (uint32_t i = 0; i < count; ++i)
                {
                    if (buffer[i].Relationship == RelationProcessorCore)
                    {
                        ++info.coreCount;
                        info.threadCount += CountSetBits(buffer[i].ProcessorMask);
                    }
                }
            }
        }
    }
}

#endif
