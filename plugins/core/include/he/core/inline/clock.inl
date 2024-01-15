// Copyright Chad Engler

namespace he
{
    template <>
    HE_FORCE_INLINE CycleCount CycleClock::Now()
    {
    #if defined(HE_PLATFORM_WASM)
        return MonotonicClock::Now();
    #elif HE_COMPILER_MSVC && HE_CPU_X86
        return { __rdtsc() };
    #elif HE_COMPILER_MSVC && HE_CPU_ARM
        return { _ReadStatusReg(ARM64_CNTVCT) };
    #elif HE_CPU_X86_32
        int64_t ret;
        asm volatile("rdtsc" : "=A"(ret));
        return { ret };
    #elif HE_CPU_X86_64
        uint64_t low;
        uint64_t high;
        asm volatile("rdtsc" : "=a"(low), "=d"(high));
        return { ((high << 32) | low) };
    #elif HE_CPU_ARM_64
        int64_t vct;
        asm volatile("mrs %0, CNTVCT_EL0" : "=r"(vct));
        return { vct };
    #elif HE_CPU_ARM
        static_assert(__ARM_ARCH >= 6, "ARMv6 is required for reading cycle counter.");

        uint32_t pmccntr;
        uint32_t pmuseren;
        uint32_t pmcntenset;

        // Read the user mode perf monitor counter access permissions.
        asm volatile("mrc p15, 0, %0, c9, c14, 0" : "=r"(pmuseren));

        // Allows reading perfmon counters for user mode code.
        if (pmuseren & 1)
        {
            asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcntenset));

            // Is it counting?
            if (pmcntenset & 0x80000000ul)
            {
                asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr));

                // The counter is set up to count every 64th cycle
                return static_cast<CycleCount>(pmccntr) * 64;
            }
        }

        // Fallback for ARM CPUs that don't let us read the cycle counter.
        return MonotonicClock::Now();
    #else
        #error "No CycleClock implementation for this architecture"
    #endif
    }
}
