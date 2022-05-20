// Copyright Chad Engler

#include "he/core/clock.h"

#include <ctime>

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
}
