// Copyright Chad Engler

#include "he/core/clock.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    struct _PerfFrequencyHelper
    {
        _PerfFrequencyHelper()
        {
            LARGE_INTEGER n;
            ::QueryPerformanceFrequency(&n);
            value = n.QuadPart;
        }

        uint64_t value;
    };

    template <>
    SystemTime SystemClock::Now()
    {
        FILETIME ft;
        ::GetSystemTimePreciseAsFileTime(&ft);

        ULARGE_INTEGER i;
        i.HighPart = ft.dwHighDateTime;
        i.LowPart = ft.dwLowDateTime;

        return Win32FileTimeToSystemTime(i.QuadPart);
    }

    template <>
    MonotonicTime MonotonicClock::Now()
    {
        static _PerfFrequencyHelper s_freq;

        LARGE_INTEGER x;
        ::QueryPerformanceCounter(&x);
        uint64_t perfTicks = static_cast<uint64_t>(x.QuadPart);

        const uint64_t ns = (perfTicks / s_freq.value) * 1000000000
            + (perfTicks % s_freq.value) * 1000000000 / s_freq.value;

        return { ns };
    }

    bool IsDaylightSavingTimeActive()
    {
        TIME_ZONE_INFORMATION tzInfo{};
        const DWORD rc = ::GetTimeZoneInformation(&tzInfo);
        return rc == TIME_ZONE_ID_DAYLIGHT;
    }

    Duration GetLocalTimezoneOffset()
    {
        TIME_ZONE_INFORMATION tzInfo{};
        const DWORD rc = ::GetTimeZoneInformation(&tzInfo);

        if (rc == TIME_ZONE_ID_INVALID)
        {
            return FromPeriod<Seconds>(0);
        }

        const LONG bias = tzInfo.Bias + (rc == TIME_ZONE_ID_DAYLIGHT ? tzInfo.DaylightBias : tzInfo.StandardBias);
        return FromPeriod<Minutes>(-bias);
    }
}

#endif
