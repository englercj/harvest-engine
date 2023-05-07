// Copyright Chad Engler

#include "he/core/clock.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/string.h"

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

    SystemTime SystemTimeFromString(StringView format, StringView value)
    {
        // Let's consider we are getting all the input in
        // this format: '2014-07-25T20:17:22Z' (T denotes
        // start of Time part, Z denotes UTC zone).
        // A better approach would be to pass in the format as well.
        static const std::wstring dateTimeFormat{ L"%Y-%m-%dT%H:%M:%SZ" };

        // Create a stream which we will use to parse the string,
        // which we provide to constructor of stream to fill the buffer.
        std::wistringstream ss{ dateTime };

        // Create a tm object to store the parsed date and time.
        std::tm dt;

        // Now we read from buffer using get_time manipulator
        // and formatting the input appropriately.
        ss >> std::get_time(&dt, dateTimeFormat.c_str());

        // Convert the tm structure to time_t value and return.
        return std::mktime(&dt);
    }
}
