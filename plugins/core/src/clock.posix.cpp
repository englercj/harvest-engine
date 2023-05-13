// Copyright Chad Engler

#include "he/core/clock.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <time.h>

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

    bool IsDaylightSavingTimeActive()
    {
        time_t t = time(nullptr);
        struct tm lt{};
        localtime_r(&t, &lt);
        return tm_isdst == 1;
    }

    Duration GetLocalTimezoneOffset()
    {
        time_t t = time(nullptr);
        struct tm lt{};
        localtime_r(&t, &lt);
        return FromPeriod<Seconds>(lt.tm_gmtoff);
    }
}

#endif
