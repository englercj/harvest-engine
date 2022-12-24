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
        m_state += 0xa0761d6478bd642full;
        return wyhash::Mix(m_state, m_state ^ 0xe7037ed1a0b428dbull);
    }

    uint64_t Random64::Next(uint64_t min, uint64_t max)
    {
        HE_ASSERT(max > min);
        uint64_t r = Next();
        uint64_t value = max - min;
        wyhash::Mum(r, value);
        return value + min;
    }

    double Random64::Real()
    {
        constexpr double Norm = 1.0 / (1ull << 52);
        const uint64_t r = Next();
        return (r >> 12) * Norm;
    }

    double Random64::Real(double min, double max)
    {
        HE_ASSERT(max > min);
        return (Real() * (max - min)) + min;
    }

    double Random64::Gauss()
    {
        constexpr double Norm = 1.0 / (1ull << 20);
        const uint64_t r = Next();
        return ((r & 0x1fffff) + ((r >> 21) & 0x1fffff) + ((r >> 42) & 0x1fffff)) * Norm - 3.0;
    }

    double Random64::Gauss(double mean, double std)
    {
        HE_ASSERT(std > 0);
        return Gauss() * std + mean;
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
