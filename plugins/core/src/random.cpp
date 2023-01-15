// Copyright Chad Engler

#include "he/core/random.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/clock.h"
#include "he/core/cpu.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"

#include "wyhash.h"

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

    Random64::Random64() noexcept
        : m_state(0)
    {
        if (!GetSecureRandomBytes(reinterpret_cast<uint8_t*>(&m_state), sizeof(m_state)))
        {
            m_state = MonotonicClock::Now().val;
        }
    }

    Random64::Random64(uint64_t seed) noexcept
        : m_state(seed)
    {}

    uint64_t Random64::Next()
    {
        return wyhash::Rand(m_state);
    }

    uint64_t Random64::Next(uint64_t min, uint64_t max)
    {
        HE_ASSERT(max > min);
        const uint64_t r = Next();
        const uint64_t k = max - min;
        return wyhash::Rand0K(r, k) + min;
    }

    double Random64::Real()
    {
        const uint64_t r = Next();
        return wyhash::Rand01(r);
    }

    double Random64::Real(double min, double max)
    {
        HE_ASSERT(max > min);
        const double r = Real();
        const double k = max - min;
        return (r * k) + min;
    }

    double Random64::Gauss()
    {
        const uint64_t r = Next();
        return wyhash::Gauss(r);
    }

    double Random64::Gauss(double mean, double std)
    {
        HE_ASSERT(std > 0);
        const double r = Gauss();
        return (r * std) + mean;
    }

    void Random64::Bytes(uint8_t* dst, uint32_t count)
    {
        while (count > sizeof(uint64_t))
        {
            const uint64_t value = Next();
            MemCopy(dst, &value, sizeof(uint64_t));

            dst += sizeof(uint64_t);
            count -= sizeof(uint64_t);
        }

        if (count > 0)
        {
            const uint64_t value = Next();
            MemCopy(dst, &value, count);
        }
    }
}
