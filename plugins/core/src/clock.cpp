// Copyright Chad Engler

#include "he/core/clock.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/string.h"

#include <iomanip>
#include <time.h>
#include <sstream>

#if HE_COMPILER_MSVC
    #define timegm _mkgmtime
#endif

namespace he
{
    constexpr uint64_t Win32FileTimeEpochOffset = 116444736000000000ull; // offset from 1/1/1601 to 1/1/1970

    SystemTime Win32FileTimeToSystemTime(uint64_t fileTime)
    {
        return { (fileTime - Win32FileTimeEpochOffset) * 100 };
    }

    uint64_t Win32FileTimeFromSystemTime(SystemTime systemTime)
    {
        return (systemTime.val / 100) + Win32FileTimeEpochOffset;
    }

    SystemTime PosixTimeToSystemTime(timespec posixTime)
    {
        return { 1000000000 * static_cast<uint64_t>(posixTime.tv_sec) + posixTime.tv_nsec };
    }

    timespec PosixTimeFromSystemTime(SystemTime systemTime)
    {
        timespec ts{};
        ts.tv_nsec = systemTime.val % 1000000000;
        ts.tv_sec = static_cast<time_t>(systemTime.val / 1000000000);
        return ts;
    }

    timespec PosixTimeFromDuration(Duration duration)
    {
        timespec ts{};
        ts.tv_nsec = duration.val % 1000000000;
        ts.tv_sec = static_cast<time_t>(duration.val / 1000000000);
        return ts;
    }

    SystemTime SystemTimeFromString(const char* format, const char* value, bool isUtc)
    {
        struct tm tm{};
        tm.tm_isdst = -1;

        std::istringstream ss(value);
        ss >> std::get_time(&tm, format);

        const time_t timeValue = isUtc ? timegm(&tm) : mktime(&tm);

        SystemTime time{};
        if (timeValue > 0)
        {
            time.val = static_cast<uint64_t>(timeValue) * Seconds::Ratio;
        }
        return time;
    }

    bool IsDaylightSavingTimeActive()
    {
        const time_t t = time(nullptr);
        struct tm tm{};
    #if HE_COMPILER_MSVC
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif
        return tm.tm_isdst > 0;
    }
}
