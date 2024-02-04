// Copyright Chad Engler

// TODO: More documentation in this file
// TODO: Rename FromPeriod -> MakeDuration & ToPeriod -> ???

#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/limits.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

struct timespec;

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Base types

    /// Base time template that represents a single time value from a clock.
    ///
    /// \note This is not a time implementation. It is a template that can be specialized to
    /// represent a time value from a clock. You probably don't want to use this directly, try using
    /// \ref SystemTime, \ref MonotonicTime, or \ref CycleCount instead.
    ///
    /// \tparam Tag A unique tag type that identifies this time.
    template <typename Tag>
    struct Time
    {
        uint64_t val;

        constexpr bool operator==(const Time& x) const { return val == x.val; }
        constexpr bool operator!=(const Time& x) const { return val != x.val; }
        constexpr bool operator<(const Time& x) const { return val < x.val; }
        constexpr bool operator<=(const Time& x) const { return val <= x.val; }
        constexpr bool operator>(const Time& x) const { return val > x.val; }
        constexpr bool operator>=(const Time& x) const { return val >= x.val; }
    };

    /// Base clock template that represents a clock that can be queried.
    ///
    /// \note This is not a clock implementation. It is a template that can be specialized to
    /// provide a clock implementation. You probably don't want to use this directly, try using
    /// \ref SystemClock, \ref MonotonicClock, or \ref CycleClock instead.
    ///
    /// \tparam Tag A unique tag type that identifies this clock.
    template <typename Tag>
    struct Clock
    {
        /// The type of time that this clock returns.
        using Time = he::Time<Tag>;

        /// Gets the current time according to this clock.
        ///
        /// \return The current time according to this clock.
        static Time Now();
    };

    // --------------------------------------------------------------------------------------------
    // Clock Types

    /// Nanoseconds that have passed since the Unix epoch (Jan 1 1970 00:00:00 UTC).
    ///
    /// These values are pulled from the system's clock which is not guaranteed to be monotonic.
    /// This clock is suitable for displaying the current time to the user.
    using SystemClock = Clock<struct SystemClockTag>;

    /// \copydoc SystemClock
    using SystemTime = SystemClock::Time;

    /// Nanoseconds that have passed since an OS-defined epoch (usually boot time).
    ///
    /// This clock is guaranteed to be monotonic. This clock is suitable for measuring precise time
    /// intervals, such as performance counters.
    using MonotonicClock = Clock<struct MonotonicClockTag>;

    /// \copydoc MonotonicClock
    using MonotonicTime = MonotonicClock::Time;

    /// Number of cycles since the last CPU reset.
    ///
    /// This clock is not guaranteed to be monotonic, though it is under certain circumstances.
    /// Values are pulled from RDTSC on x86 and CNTVCT on ARM. This means that any kind of
    /// interrupt (including scheduling preemption) can result in unexpected values.
    /// This clock is suitable for measuring very precise time intervals, such as performance
    /// counters, with the caveats mentioned before.
    using CycleClock = Clock<struct CycleClockTag>;

    /// \copydoc CycleClock
    using CycleCount = CycleClock::Time;

    // --------------------------------------------------------------------------------------------
    // Duration Types

    // A span of time in nanoseconds
    struct Duration
    {
        int64_t val;

        constexpr bool operator==(const Duration& x) const { return val == x.val; }
        constexpr bool operator!=(const Duration& x) const { return val != x.val; }
        constexpr bool operator<(const Duration& x) const { return val < x.val; }
        constexpr bool operator<=(const Duration& x) const { return val <= x.val; }
        constexpr bool operator>(const Duration& x) const { return val > x.val; }
        constexpr bool operator>=(const Duration& x) const { return val >= x.val; }
    };

    inline constexpr Duration Duration_Zero{ 0 };
    inline constexpr Duration Duration_Min{ Limits<int64_t>::Min };
    inline constexpr Duration Duration_Max{ Limits<int64_t>::Max };

    template <int64_t N> struct DurationPeriod { static constexpr int64_t Ratio = N; };
    using Nanoseconds = DurationPeriod<1>;
    using Microseconds = DurationPeriod<1000>;
    using Milliseconds = DurationPeriod<1000000>;
    using Seconds = DurationPeriod<1000000000>;
    using Minutes = DurationPeriod<60000000000>;
    using Hours = DurationPeriod<3600000000000>;
    using Days = DurationPeriod<86400000000000>;
    using Weeks = DurationPeriod<604800000000000>;

    template <typename T, typename U = int64_t> requires(IsIntegral<U>)
    constexpr U ToPeriod(Duration d) { return static_cast<U>(d.val / T::Ratio); }

    template <typename T, typename U> requires(IsFloatingPoint<U>)
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

    // --------------------------------------------------------------------------------------------
    // System time converters

    /// Converts a Win32 file time to a \ref SystemTime value that represents the same moment
    /// in time.
    ///
    /// \param[in] systemTime The \ref SystemTime to convert.
    /// \return The converted \ref SystemTime value.
    SystemTime Win32FileTimeToSystemTime(uint64_t fileTime);

    /// Converts a \ref SystemTime to a Win32 file time value that represents the same moment
    /// in time.
    ///
    /// \param[in] systemTime The \ref SystemTime to convert.
    /// \return The converted Win32 file time value.
    uint64_t Win32FileTimeFromSystemTime(SystemTime systemTime);

    /// Converts a Posix timespec to a \ref SystemTime value that represents the same moment
    /// in time.
    ///
    /// \param[in] posixTime The Posix timespec to convert.
    /// \return The converted \ref SystemTime value.
    SystemTime PosixTimeToSystemTime(timespec posixTime);

    /// Converts a \ref SystemTime to a Posix timespec value that represents the same moment
    /// in time.
    ///
    /// \param[in] systemTime The \ref SystemTime to convert.
    /// \return The converted Posix timespec value.
    timespec PosixTimeFromSystemTime(SystemTime systemTime);

    /// Converts a \ref Duration to a Posix timespec value that represents the same duration.
    ///
    /// \param[in] duration The \ref Duration to convert.
    /// \return The converted Posix timespec value.
    timespec PosixTimeFromDuration(Duration duration);

    /// Checks if Daylight Saving Time is active right now in the current locale.
    ///
    /// \return `true` if Daylight Saving Time is active, `false` otherwise.
    bool IsDaylightSavingTimeActive();

    /// Gets the local timezone offset as a \ref Duration.
    ///
    /// \return The local timezone offset \ref Duration.
    Duration GetLocalTimezoneOffset();

    // --------------------------------------------------------------------------------------------
    // User-defined literals

    /// User-defined literal that creates a Duration of nanoseconds from an integer literal.
    ///
    /// Example:
    /// ```cpp
    /// const Duration value = 125_ns;
    /// ```
    ///
    /// \param[in] value The integer literal.
    /// \return A Duration of `value` nanoseconds.
    [[nodiscard]] constexpr Duration operator"" _ns(unsigned long long value) noexcept
    {
        return FromPeriod<Nanoseconds>(value);
    }

    /// User-defined literal that creates a Duration of microseconds from an integer literal.
    ///
    /// Example:
    /// ```cpp
    /// const Duration value = 125_us;
    /// ```
    ///
    /// \param[in] value The integer literal.
    /// \return A Duration of `value` microseconds.
    [[nodiscard]] constexpr Duration operator"" _us(unsigned long long value) noexcept
    {
        return FromPeriod<Microseconds>(value);
    }

    /// User-defined literal that creates a Duration of milliseconds from an integer literal.
    ///
    /// Example:
    /// ```cpp
    /// const Duration value = 125_ms;
    /// ```
    ///
    /// \param[in] value The integer literal.
    /// \return A Duration of `value` milliseconds.
    [[nodiscard]] constexpr Duration operator"" _ms(unsigned long long value) noexcept
    {
        return FromPeriod<Milliseconds>(value);
    }

    /// User-defined literal that creates a Duration of seconds from an integer literal.
    ///
    /// Example:
    /// ```cpp
    /// const Duration value = 125_sec;
    /// ```
    ///
    /// \param[in] value The integer literal.
    /// \return A Duration of `value` seconds.
    [[nodiscard]] constexpr Duration operator"" _sec(unsigned long long value) noexcept
    {
        return FromPeriod<Seconds>(value);
    }

    /// User-defined literal that creates a Duration of minutes from an integer literal.
    ///
    /// Example:
    /// ```cpp
    /// const Duration value = 125_min;
    /// ```
    ///
    /// \param[in] value The integer literal.
    /// \return A Duration of `value` minutes.
    [[nodiscard]] constexpr Duration operator"" _min(unsigned long long value) noexcept
    {
        return FromPeriod<Minutes>(value);
    }

    /// User-defined literal that creates a Duration of hours from an integer literal.
    ///
    /// Example:
    /// ```cpp
    /// const Duration value = 125_hour;
    /// ```
    ///
    /// \param[in] value The integer literal.
    /// \return A Duration of `value` hours.
    [[nodiscard]] constexpr Duration operator"" _hour(unsigned long long value) noexcept
    {
        return FromPeriod<Hours>(value);
    }

    /// User-defined literal that creates a Duration of days from an integer literal.
    ///
    /// Example:
    /// ```cpp
    /// const Duration value = 125_day;
    /// ```
    ///
    /// \param[in] value The integer literal.
    /// \return A Duration of `value` days.
    [[nodiscard]] constexpr Duration operator"" _day(unsigned long long value) noexcept
    {
        return FromPeriod<Days>(value);
    }

    /// User-defined literal that creates a Duration of weeks from an integer literal.
    ///
    /// Example:
    /// ```cpp
    /// const Duration value = 125_week;
    /// ```
    ///
    /// \param[in] value The integer literal.
    /// \return A Duration of `value` weeks.
    [[nodiscard]] constexpr Duration operator"" _week(unsigned long long value) noexcept
    {
        return FromPeriod<Weeks>(value);
    }
}

#include "he/core/inline/clock.inl"
