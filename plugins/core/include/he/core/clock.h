// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/types.h"

#include <type_traits>

#if HE_COMPILER_MSVC && HE_CPU_X86
    extern "C" uint64_t __rdtsc();
    #pragma intrinsic(__rdtsc)
#endif

struct timespec;

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Base types

    template <typename Tag>
    struct Time
    {
        uint64_t val;
    };

    template <typename Tag>
    struct Clock
    {
        using Time = Time<Tag>;
        static Time Now();
    };

    // --------------------------------------------------------------------------------------------
    // Implementations

    // Nanoseconds that have passed since the Unix epoch (Jan 1 1970 00:00:00).
    // These values are pulled from the system's clock which is not monotonic.
    struct SystemClockTag;
    using SystemClock = Clock<SystemClockTag>;
    using SystemTime = SystemClock::Time;

    // Nanoseconds that have passed since an OS-defined epoch (usually boot time).
    // This clock is guaranteed to be monotonic.
    struct MonotonicClockTag;
    using MonotonicClock = Clock<MonotonicClockTag>;
    using MonotonicTime = MonotonicClock::Time;

    // Number of cycles since the last CPU reset.
    // This clock is not guaranteed to be monotonic.
    struct CycleClockTag;
    using CycleClock = Clock<CycleClockTag>;
    using CycleCount = CycleClock::Time;

    // --------------------------------------------------------------------------------------------
    // Durations

    // A span of time in nanoseconds
    struct Duration { int64_t val; };

    constexpr Duration Duration_Zero{ 0 };
    constexpr Duration Duration_Min{ INT64_MIN };
    constexpr Duration Duration_Max{ INT64_MAX };

    // --------------------------------------------------------------------------------------------
    // Duration periods

    template <int64_t N> struct DurationPeriod { static constexpr int64_t Ratio = N; };
    using Nanoseconds = DurationPeriod<1>;
    using Microseconds = DurationPeriod<1000>;
    using Milliseconds = DurationPeriod<1000000>;
    using Seconds = DurationPeriod<1000000000>;
    using Minutes = DurationPeriod<60000000000>;
    using Hours = DurationPeriod<3600000000000>;
    using Days = DurationPeriod<86400000000000>;
    using Weeks = DurationPeriod<604800000000000>;

    template <typename T, typename U = int64_t> requires(std::is_integral_v<U>)
    constexpr U ToPeriod(Duration d) { return static_cast<U>(d.val / T::Ratio); }

    template <typename T, typename U> requires(std::is_floating_point_v<U>)
    constexpr U ToPeriod(Duration d) { return static_cast<U>(d.val / static_cast<double>(T::Ratio)); }

    template <typename T, typename U>
    constexpr Duration FromPeriod(U p) { return { static_cast<int64_t>(p * T::Ratio) }; }

    // --------------------------------------------------------------------------------------------
    // Time & Duration operators

    template <typename Tag> inline Time<Tag>& operator+=(Time<Tag>& x, Duration y) { x.val += y.val; return x; }
    template <typename Tag> constexpr Time<Tag> operator+(Time<Tag> x, Duration y) { return { x.val + y.val }; }
    template <typename Tag> constexpr Time<Tag> operator+(Duration x, Time<Tag> y) { return { x.val + y.val }; }

    inline Duration& operator+=(Duration& x, Duration y) { x.val += y.val; return x; }
    constexpr Duration operator+(Duration x, Duration y) { return { x.val + y.val }; }

    template <typename Tag> inline Time<Tag>& operator-=(Time<Tag>& x, Duration y) { x.val -= y.val; return x; }
    template <typename Tag> constexpr Duration operator-(Time<Tag> x, Time<Tag> y) { return { static_cast<int64_t>(x.val - y.val) }; }
    template <typename Tag> constexpr Time<Tag> operator-(Time<Tag> x, Duration y) { return { x.val - y.val }; }

    inline Duration& operator-=(Duration& x, Duration y) { x.val -= y.val; return x; }
    constexpr Duration operator-(Duration x, Duration y) { return { x.val - y.val }; }

    template <typename Tag> constexpr bool operator==(Time<Tag> x, Time<Tag> y) { return x.val == y.val; }
    template <typename Tag> constexpr bool operator!=(Time<Tag> x, Time<Tag> y) { return x.val != y.val; }
    template <typename Tag> constexpr bool operator<(Time<Tag> x, Time<Tag> y)  { return x.val <  y.val; }
    template <typename Tag> constexpr bool operator<=(Time<Tag> x, Time<Tag> y) { return x.val <= y.val; }
    template <typename Tag> constexpr bool operator>(Time<Tag> x, Time<Tag> y)  { return x.val >  y.val; }
    template <typename Tag> constexpr bool operator>=(Time<Tag> x, Time<Tag> y) { return x.val >= y.val; }

    constexpr bool operator==(Duration x, Duration y)   { return x.val == y.val; }
    constexpr bool operator!=(Duration x, Duration y)   { return x.val != y.val; }
    constexpr bool operator<(Duration x, Duration y)    { return x.val <  y.val; }
    constexpr bool operator<=(Duration x, Duration y)   { return x.val <= y.val; }
    constexpr bool operator>(Duration x, Duration y)    { return x.val >  y.val; }
    constexpr bool operator>=(Duration x, Duration y)   { return x.val >= y.val; }

    // --------------------------------------------------------------------------------------------
    // System time converters

    // Converts to and from a windows file times
    SystemTime Win32FileTimeToSystemTime(uint64_t fileTime);
    uint64_t Win32FileTimeFromSystemTime(SystemTime systemTime);

    // Converts to and from posix times
    SystemTime PosixTimeToSystemTime(timespec posixTime);
    timespec PosixTimeFromSystemTime(SystemTime systemTime);

    // --------------------------------------------------------------------------------------------
    // CycleClock inline implementation

    template <>
    HE_FORCE_INLINE CycleCount CycleClock::Now()
    {
    #if defined(HE_PLATFORM_EMSCRIPTEN)
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
        return { (high << 32) | low) };
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
