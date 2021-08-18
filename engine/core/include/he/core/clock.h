// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

struct timespec;

namespace he
{
    template <typename Tag> struct Time { uint64_t ns; };

    template <typename Tag> struct Clock
    {
        using Time = he::Time<Tag>;

        static Time Now();
    };

    // Nanoseconds that have passed since the Unix epoch (Jan 1 1970 00:00:00).
    // These values are pulled from the system's clock which is not monotonic.
    struct SystemClockTag;
    using SystemClock = Clock<SystemClockTag>;
    using SystemTime = SystemClock::Time;

    // Nanoseconds that have passed since an OS-defined epoch (usually boot time).
    // This clock is garuanteed to be monotonic.
    struct MonotonicClockTag;
    using MonotonicClock = Clock<MonotonicClockTag>;
    using MonotonicTime = MonotonicClock::Time;

    // A span of time in nanoseconds
    struct Duration { int64_t ns; };

    constexpr Duration Duration_Zero{ 0 };
    constexpr Duration Duration_Max{ INT64_MAX };
    constexpr Duration Duration_Min{ INT64_MIN };

    // Duration periods
    template <int64_t N> struct DurationPeriod { static constexpr int64_t Ratio = N; };
    using Nanoseconds = DurationPeriod<1>;
    using Microseconds = DurationPeriod<1000>;
    using Milliseconds = DurationPeriod<1000000>;
    using Seconds = DurationPeriod<1000000000>;
    using Minutes = DurationPeriod<60000000000>;
    using Hours = DurationPeriod<3600000000000>;

    template <typename T>
    constexpr int64_t ToPeriod(Duration d) { return d.ns / T::Ratio; }

    template <typename T>
    constexpr float ToFloatPeriod(Duration d) { return static_cast<float>(d.ns / static_cast<double>(T::Ratio)); }

    template <typename T, typename U>
    constexpr Duration FromPeriod(U p) { return { p * T::Ratio }; }

    // Time & Duration operators
    template <typename Tag> inline Time<Tag>& operator+=(Time<Tag>& x, Duration y) { x.ns += y.ns; return x; }
    template <typename Tag> constexpr Time<Tag> operator+(Time<Tag> x, Duration y) { return { x.ns + y.ns }; }
    template <typename Tag> constexpr Time<Tag> operator+(Duration x, Time<Tag> y) { return { x.ns + y.ns }; }

    inline Duration& operator+=(Duration& x, Duration y) { x.ns += y.ns; return x; }
    constexpr Duration operator+(Duration x, Duration y) { return { x.ns + y.ns }; }

    template <typename Tag> inline Time<Tag>& operator-=(Time<Tag>& x, Duration y) { x.ns -= y.ns; return x; }
    template <typename Tag> constexpr Duration operator-(Time<Tag> x, Time<Tag> y) { return { static_cast<int64_t>(x.ns - y.ns) }; }
    template <typename Tag> constexpr Time<Tag> operator-(Time<Tag> x, Duration y) { return { x.ns - y.ns }; }

    inline Duration& operator-=(Duration& x, Duration y) { x.ns -= y.ns; return x; }
    constexpr Duration operator-(Duration x, Duration y) { return { x.ns - y.ns }; }

    template <typename Tag> constexpr bool operator==(Time<Tag> x, Time<Tag> y) { return x.ns == y.ns; }
    template <typename Tag> constexpr bool operator!=(Time<Tag> x, Time<Tag> y) { return x.ns != y.ns; }
    template <typename Tag> constexpr bool operator<(Time<Tag> x, Time<Tag> y)  { return x.ns <  y.ns; }
    template <typename Tag> constexpr bool operator<=(Time<Tag> x, Time<Tag> y) { return x.ns <= y.ns; }
    template <typename Tag> constexpr bool operator>(Time<Tag> x, Time<Tag> y)  { return x.ns >  y.ns; }
    template <typename Tag> constexpr bool operator>=(Time<Tag> x, Time<Tag> y) { return x.ns >= y.ns; }

    constexpr bool operator==(Duration x, Duration y)   { return x.ns == y.ns; }
    constexpr bool operator!=(Duration x, Duration y)   { return x.ns != y.ns; }
    constexpr bool operator<(Duration x, Duration y)    { return x.ns <  y.ns; }
    constexpr bool operator<=(Duration x, Duration y)   { return x.ns <= y.ns; }
    constexpr bool operator>(Duration x, Duration y)    { return x.ns >  y.ns; }
    constexpr bool operator>=(Duration x, Duration y)   { return x.ns >= y.ns; }

    // Converts to and from a windows file times
    SystemTime Win32FileTimeToSystemTime(uint64_t fileTime);
    uint64_t Win32FileTimeFromSystemTime(SystemTime systemTime);

    // Converts to and from posix times
    SystemTime PosixTimeToSystemTime(timespec posixTime);
    timespec PosixTimeFromSystemTime(SystemTime systemTime);
}
