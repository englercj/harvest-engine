// Copyright Chad Engler

#include "he/core/clock.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    SystemTime SystemClock::Now()
    {
        FILETIME ft;
        ::GetSystemTimePreciseAsFileTime(&ft);

        ULARGE_INTEGER i;
        i.HighPart = ft.dwHighDateTime;
        i.LowPart = ft.dwLowDateTime;

        return Win32FileTimeToSystemTime(i.QuadPart);
    }

    MonotonicTime MonotonicClock::Now()
    {
        LARGE_INTEGER x;
        ::QueryPerformanceCounter(&x);
        uint64_t perfTicks = static_cast<uint64_t>(x.QuadPart);

        static uint64_t s_freq = 0;
        if (s_freq == 0)
        {
            LARGE_INTEGER n;
            ::QueryPerformanceFrequency(&n);
            s_freq = n.QuadPart;
        }

        const uint64_t ns = (perfTicks / s_freq) * 1000000000
            + (perfTicks % s_freq) * 1000000000 / s_freq;

        return { ns };
    }
}

#endif
