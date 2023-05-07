// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/limits.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

#if HE_COMPILER_MSVC && HE_CPU_X86
    extern "C" unsigned __int64 __rdtsc();
    #pragma intrinsic(__rdtsc)
#elif HE_COMPILER_MSVC && HE_CPU_ARM
    extern "C" __int64 _ReadStatusReg(int);
    #pragma intrinsic(_ReadStatusReg)
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
        using Time = he::Time<Tag>;
        static Time Now();
    };

    // --------------------------------------------------------------------------------------------
    // Implementations

    // Nanoseconds that have passed since the Unix epoch (Jan 1 1970 00:00:00 UTC).
    // These values are pulled from the system's clock which is not monotonic.
    using SystemClock = Clock<struct SystemClockTag>;
    using SystemTime = SystemClock::Time;

    // Nanoseconds that have passed since an OS-defined epoch (usually boot time).
    // This clock is guaranteed to be monotonic.
    using MonotonicClock = Clock<struct MonotonicClockTag>;
    using MonotonicTime = MonotonicClock::Time;

    // Number of cycles since the last CPU reset.
    // This clock is not guaranteed to be monotonic.
    using CycleClock = Clock<struct CycleClockTag>;
    using CycleCount = CycleClock::Time;

    // --------------------------------------------------------------------------------------------
    // Durations

    // A span of time in nanoseconds
    struct Duration { int64_t val; };

    constexpr Duration Duration_Zero{ 0 };
    constexpr Duration Duration_Min{ Limits<int64_t>::Min };
    constexpr Duration Duration_Max{ Limits<int64_t>::Max };

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

    /// Converts a date-time string to a \ref SystemTime value.
    ///
    /// \param[in] format The format string to use for parsing.
    /// \param[in] value The date-time string to parse.
    /// \return The converted \ref SystemTime value.
    SystemTime SystemTimeFromString(const char* format, const char* value);

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
