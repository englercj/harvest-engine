// Copyright Chad Engler

#include "he/core/clock.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <ctime>

namespace he
{
    template <>
    SystemTime SystemClock::Now()
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return PosixTimeToSystemTime(ts);
    }

    template <>
    MonotonicTime MonotonicClock::Now()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return { static_cast<uint64_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec };
    }
}

#endif
