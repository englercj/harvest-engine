// Copyright Chad Engler

#include "he/core/random.h"

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"

#if HE_CPU_X86
    #include "rdrand.x86.h"
#endif

namespace he
{
#if HE_CPU_X86
    #define HE_HAS_HW_RAND(info) info.x86.rdrand
    #define HE_HW_RAND(v) _RdRand(v)
#elif HE_CPU_ARM
    #define HE_HAS_HW_RAND(info) info.arm.rndr
    #define HE_HW_RAND(v) _Rndr(v)
#endif

    bool GetSecureRandomBytes(uint8_t* dst, size_t count)
    {
        if (!GetHardwareRandomBytes(dst, count))
            return GetSystemRandomBytes(dst, count);

        return true;
    }

    bool GetHardwareRandomBytes(uint8_t* dst, size_t count)
    {
        const CpuInfo& cpuInfo = GetCpuInfo();
        if (!HE_HAS_HW_RAND(cpuInfo))
            return false;

        while (count > sizeof(size_t))
        {
            size_t value;
            if (!HE_HW_RAND(value))
                return false;

            MemCopy(dst, &value, sizeof(size_t));

            dst += sizeof(size_t);
            count -= sizeof(size_t);
        }

        if (count > 0)
        {
            size_t value;
            if (!HE_HW_RAND(value))
                return false;

            MemCopy(dst, &value, count);
        }

        return true;
    }
}
