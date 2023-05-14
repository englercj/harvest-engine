// Copyright Chad Engler

#include "he/core/clock.h"
#include "he/core/clock_fmt.h"

#include "he/core/fmt.h"
#include "he/core/type_traits.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, SystemClock)
{
    static_assert(IsSame<SystemClock::Time, SystemTime>);
    SystemTime t = SystemClock::Now();
    HE_EXPECT_GT(t.val, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, MonotonicClock)
{
    static_assert(IsSame<MonotonicClock::Time, MonotonicTime>);
    MonotonicTime t = MonotonicClock::Now();
    HE_EXPECT_GT(t.val, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, CycleClock)
{
    static_assert(IsSame<CycleClock::Time, CycleCount>);
    CycleCount t = CycleClock::Now();
    HE_EXPECT_GT(t.val, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, Duration)
{
    static_assert(Duration_Zero.val == 0);
    static_assert(Duration_Max.val == INT64_MAX);
    static_assert(Duration_Min.val == INT64_MIN);

    HE_EXPECT_EQ(Duration_Zero.val, 0);
    HE_EXPECT_EQ(Duration_Max.val, INT64_MAX);
    HE_EXPECT_EQ(Duration_Min.val, INT64_MIN);

    HE_EXPECT_EQ(Duration{ 15 }.val, 15);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, DurationPeriod)
{
    static_assert(Nanoseconds::Ratio == 1);
    static_assert(Microseconds::Ratio == 1000);
    static_assert(Milliseconds::Ratio == 1000000);
    static_assert(Seconds::Ratio == 1000000000);
    static_assert(Minutes::Ratio == 60000000000);
    static_assert(Hours::Ratio == 3600000000000);
    static_assert(Days::Ratio == 86400000000000);
    static_assert(Weeks::Ratio == 604800000000000);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, ToPeriod)
{
    // As int64_t (default)

    static_assert(ToPeriod<Nanoseconds>(Duration_Zero) == 0);
    static_assert(ToPeriod<Microseconds>(Duration_Zero) == 0);
    static_assert(ToPeriod<Milliseconds>(Duration_Zero) == 0);
    static_assert(ToPeriod<Seconds>(Duration_Zero) == 0);
    static_assert(ToPeriod<Minutes>(Duration_Zero) == 0);
    static_assert(ToPeriod<Hours>(Duration_Zero) == 0);
    static_assert(ToPeriod<Days>(Duration_Zero) == 0);
    static_assert(ToPeriod<Weeks>(Duration_Zero) == 0);

    static_assert(ToPeriod<Nanoseconds>(Duration{ 2000 }) == 2000);
    static_assert(ToPeriod<Microseconds>(Duration{ 2000 }) == 2);
    static_assert(ToPeriod<Milliseconds>(Duration{ 2000 }) == 0);
    static_assert(ToPeriod<Seconds>(Duration{ 2000 }) == 0);
    static_assert(ToPeriod<Minutes>(Duration{ 2000 }) == 0);
    static_assert(ToPeriod<Hours>(Duration{ 2000 }) == 0);
    static_assert(ToPeriod<Days>(Duration{ 2000 }) == 0);
    static_assert(ToPeriod<Weeks>(Duration{ 2000 }) == 0);

    HE_EXPECT_EQ(ToPeriod<Nanoseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Microseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Milliseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Seconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Minutes>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Hours>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Days>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Weeks>(Duration_Zero), 0);

    HE_EXPECT_EQ(ToPeriod<Nanoseconds>(Duration{ 2000 }), 2000);
    HE_EXPECT_EQ(ToPeriod<Microseconds>(Duration{ 2000 }), 2);
    HE_EXPECT_EQ(ToPeriod<Milliseconds>(Duration{ 2000 }), 0);
    HE_EXPECT_EQ(ToPeriod<Seconds>(Duration{ 2000 }), 0);
    HE_EXPECT_EQ(ToPeriod<Minutes>(Duration{ 2000 }), 0);
    HE_EXPECT_EQ(ToPeriod<Hours>(Duration{ 2000 }), 0);
    HE_EXPECT_EQ(ToPeriod<Days>(Duration{ 2000 }), 0);
    HE_EXPECT_EQ(ToPeriod<Weeks>(Duration{ 2000 }), 0);

    // As double

    static_assert(ToPeriod<Nanoseconds, double>(Duration_Zero) == 0.0);
    static_assert(ToPeriod<Microseconds, double>(Duration_Zero) == 0.0);
    static_assert(ToPeriod<Milliseconds, double>(Duration_Zero) == 0.0);
    static_assert(ToPeriod<Seconds, double>(Duration_Zero) == 0.0);
    static_assert(ToPeriod<Minutes, double>(Duration_Zero) == 0.0);
    static_assert(ToPeriod<Hours, double>(Duration_Zero) == 0.0);
    static_assert(ToPeriod<Days, double>(Duration_Zero) == 0.0);
    static_assert(ToPeriod<Weeks, double>(Duration_Zero) == 0.0);

    static_assert(ToPeriod<Nanoseconds, double>(Duration{ 2000 }) == 2000.0);
    static_assert(ToPeriod<Microseconds, double>(Duration{ 2000 }) == 2.0);
    static_assert(EqualUlp(ToPeriod<Milliseconds, double>(Duration{ 2000 }), 0.002, 1));
    static_assert(EqualUlp(ToPeriod<Seconds, double>(Duration{ 2000 }), 0.000002, 1));

    HE_EXPECT_EQ((ToPeriod<Nanoseconds, double>(Duration_Zero)), 0.0);
    HE_EXPECT_EQ((ToPeriod<Microseconds, double>(Duration_Zero)), 0.0);
    HE_EXPECT_EQ((ToPeriod<Milliseconds, double>(Duration_Zero)), 0.0);
    HE_EXPECT_EQ((ToPeriod<Seconds, double>(Duration_Zero)), 0.0);
    HE_EXPECT_EQ((ToPeriod<Minutes, double>(Duration_Zero)), 0.0);
    HE_EXPECT_EQ((ToPeriod<Hours, double>(Duration_Zero)), 0.0);
    HE_EXPECT_EQ((ToPeriod<Days, double>(Duration_Zero)), 0.0);
    HE_EXPECT_EQ((ToPeriod<Weeks, double>(Duration_Zero)), 0.0);

    HE_EXPECT_EQ((ToPeriod<Nanoseconds, double>(Duration{ 2000 })), 2000.0);
    HE_EXPECT_EQ((ToPeriod<Microseconds, double>(Duration{ 2000 })), 2.0);
    HE_EXPECT_EQ_ULP((ToPeriod<Milliseconds, double>(Duration{ 2000 })), 0.002, 1);
    HE_EXPECT_EQ_ULP((ToPeriod<Seconds, double>(Duration{ 2000 })), 0.000002, 1);

    // As float

    static_assert(ToPeriod<Nanoseconds, float>(Duration_Zero) == 0.0f);
    static_assert(ToPeriod<Microseconds, float>(Duration_Zero) == 0.0f);
    static_assert(ToPeriod<Milliseconds, float>(Duration_Zero) == 0.0f);
    static_assert(ToPeriod<Seconds, float>(Duration_Zero) == 0.0f);
    static_assert(ToPeriod<Minutes, float>(Duration_Zero) == 0.0f);
    static_assert(ToPeriod<Hours, float>(Duration_Zero) == 0.0f);
    static_assert(ToPeriod<Days, float>(Duration_Zero) == 0.0f);
    static_assert(ToPeriod<Weeks, float>(Duration_Zero) == 0.0f);

    static_assert(ToPeriod<Nanoseconds, float>(Duration{ 2000 }) == 2000.0f);
    static_assert(ToPeriod<Microseconds, float>(Duration{ 2000 }) == 2.0f);
    static_assert(EqualUlp(ToPeriod<Milliseconds, float>(Duration{ 2000 }), 0.002f, 1));
    static_assert(EqualUlp(ToPeriod<Seconds, float>(Duration{ 2000 }), 0.000002f, 1));

    HE_EXPECT_EQ((ToPeriod<Nanoseconds, float>(Duration_Zero)), 0.0f);
    HE_EXPECT_EQ((ToPeriod<Microseconds, float>(Duration_Zero)), 0.0f);
    HE_EXPECT_EQ((ToPeriod<Milliseconds, float>(Duration_Zero)), 0.0f);
    HE_EXPECT_EQ((ToPeriod<Seconds, float>(Duration_Zero)), 0.0f);
    HE_EXPECT_EQ((ToPeriod<Minutes, float>(Duration_Zero)), 0.0f);
    HE_EXPECT_EQ((ToPeriod<Hours, float>(Duration_Zero)), 0.0f);
    HE_EXPECT_EQ((ToPeriod<Days, float>(Duration_Zero)), 0.0f);
    HE_EXPECT_EQ((ToPeriod<Weeks, float>(Duration_Zero)), 0.0f);

    HE_EXPECT_EQ((ToPeriod<Nanoseconds, float>(Duration{ 2000 })), 2000.0f);
    HE_EXPECT_EQ((ToPeriod<Microseconds, float>(Duration{ 2000 })), 2.0f);
    HE_EXPECT_EQ_ULP((ToPeriod<Milliseconds, float>(Duration{ 2000 })), 0.002f, 1);
    HE_EXPECT_EQ_ULP((ToPeriod<Seconds, float>(Duration{ 2000 })), 0.000002f, 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, FromPeriod)
{
    static_assert(FromPeriod<Nanoseconds>(0) == Duration_Zero);
    static_assert(FromPeriod<Microseconds>(0) == Duration_Zero);
    static_assert(FromPeriod<Milliseconds>(0) == Duration_Zero);
    static_assert(FromPeriod<Seconds>(0) == Duration_Zero);
    static_assert(FromPeriod<Minutes>(0) == Duration_Zero);
    static_assert(FromPeriod<Hours>(0) == Duration_Zero);

    static_assert(FromPeriod<Nanoseconds>(2000.0f) == Duration{ 2000 });
    static_assert(FromPeriod<Microseconds>(2.0f) == Duration{ 2000 });
    static_assert(FromPeriod<Milliseconds>(0.002f) == Duration{ 2000 });
    //static_assert(FromPeriod<Seconds>(0.000002f) == Duration{ 2000 });

    HE_EXPECT_EQ(FromPeriod<Nanoseconds>(0), Duration_Zero);
    HE_EXPECT_EQ(FromPeriod<Microseconds>(0), Duration_Zero);
    HE_EXPECT_EQ(FromPeriod<Milliseconds>(0), Duration_Zero);
    HE_EXPECT_EQ(FromPeriod<Seconds>(0), Duration_Zero);
    HE_EXPECT_EQ(FromPeriod<Minutes>(0), Duration_Zero);
    HE_EXPECT_EQ(FromPeriod<Hours>(0), Duration_Zero);

    HE_EXPECT_EQ(FromPeriod<Nanoseconds>(2000.0f), Duration{ 2000 });
    HE_EXPECT_EQ(FromPeriod<Microseconds>(2.0f), Duration{ 2000 });
    HE_EXPECT_EQ(FromPeriod<Milliseconds>(0.002f), Duration{ 2000 });
    HE_EXPECT_EQ(FromPeriod<Seconds>(0.000002f), Duration{ 2000 });
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, operators_Arithmatic)
{
    struct Tag;
    using T = Time<Tag>;

    T t0{ 567 };
    T t1{ 567 };
    Duration d0{ 123 };
    Duration d1{ 123 };

    // T += D
    T a = t0;
    a += d0;
    HE_EXPECT_EQ(a.val, 690);

    // T + D
    HE_EXPECT_EQ((t1 + d0).val, 690);

    // D + T
    HE_EXPECT_EQ((d0 + t1).val, 690);

    // D += D
    Duration b = d0;
    b += d1;
    HE_EXPECT_EQ(b, Duration{ 246 });

    // D + D
    HE_EXPECT_EQ((d0 + d1), Duration{ 246 });

    // T -= D
    T c = t0;
    c -= d0;
    HE_EXPECT_EQ(c.val, 444);

    // T - T
    HE_EXPECT_EQ((t1 - t0), Duration{ 0 });

    // T - D
    HE_EXPECT_EQ((t0 - d0).val, 444);

    // D -= D
    Duration d = d0;
    d -= d1;
    HE_EXPECT_EQ(d, Duration{ 0 });

    // D - D
    HE_EXPECT_EQ((d0 - d1), Duration{ 0 });
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, operators_Comparison)
{
    struct Tag;
    using T = Time<Tag>;

    T t0{ 123 };
    T t1{ 456 };

    HE_EXPECT((t0 == t0));
    HE_EXPECT((t0 != t1));
    HE_EXPECT((t0 < t1));
    HE_EXPECT((t0 <= t0));
    HE_EXPECT((t0 <= t1));
    HE_EXPECT((t1 > t0));
    HE_EXPECT((t1 >= t1));
    HE_EXPECT((t1 >= t0));

    Duration d0{ 123 };
    Duration d1{ 456 };

    HE_EXPECT((d0 == d0));
    HE_EXPECT((d0 != d1));
    HE_EXPECT((d0 < d1));
    HE_EXPECT((d0 <= d0));
    HE_EXPECT((d0 <= d1));
    HE_EXPECT((d1 > d0));
    HE_EXPECT((d1 >= d1));
    HE_EXPECT((d1 >= d0));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, Win32FileTimeToSystemTime)
{
    uint64_t t = 116444736000000000;
    HE_EXPECT_EQ(Win32FileTimeToSystemTime(t).val, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, Win32FileTimeFromSystemTime)
{
    SystemTime t{ 0 };
    HE_EXPECT_EQ(Win32FileTimeFromSystemTime(t), 116444736000000000);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, PosixTimeToSystemTime)
{
    timespec posixTime;
    posixTime.tv_nsec = 987654321;
    posixTime.tv_sec = 123456789;
    HE_EXPECT_EQ(PosixTimeToSystemTime(posixTime).val, 123456789987654321);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, PosixTimeFromSystemTime)
{
    SystemTime systemTime{ 123456789987654321 };
    auto test = PosixTimeFromSystemTime(systemTime);
    HE_EXPECT_EQ(test.tv_nsec, 987654321);
    HE_EXPECT_EQ(test.tv_sec, 123456789);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, PosixTimeFromDuration)
{
    Duration systemTime{ 60000000123 };
    auto test = PosixTimeFromDuration(systemTime);
    HE_EXPECT_EQ(test.tv_nsec, 123);
    HE_EXPECT_EQ(test.tv_sec, 60);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, IsDaylightSavingTimeActive)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, GetLocalTimezoneOffset)
{
    // TODO: set the local timezone to -07:00 for these to pass
    const Duration tzOffset = GetLocalTimezoneOffset();
    HE_EXPECT_EQ(tzOffset, FromPeriod<Hours>(-7));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, fmt_time)
{
    // 8:15 AM, Thursday, May 19, 2022 (PDT)
    SystemTime time{ 1652973325730225600 };

    // Default format
    {
        String actual = Format("{}", time);
        HE_EXPECT_EQ(actual, "2022-05-19T15:15:25Z");
    }

    // Default format - utc
    {
        String actual = Format("{}", FmtUtcTime(time));
        HE_EXPECT_EQ(actual, "2022-05-19T15:15:25Z");
    }

    // Default format - local
    {
        String actual = Format("{}", FmtLocalTime(time));
        HE_EXPECT_EQ(actual, "2022-05-19T08:15:25-0700");
    }

    // Custom format
    {
        String actual = Format("{:%A %b %d, %y %T %p}", time);
        HE_EXPECT_EQ(actual, "Thursday May 19, 22 15:15:25 PM");
    }

    // Custom format - utc
    {
        String actual = Format("{:%A %b %d, %y %T %p}", FmtUtcTime(time));
        HE_EXPECT_EQ(actual, "Thursday May 19, 22 15:15:25 PM");
    }

    // Custom format - local
    {
        String actual = Format("{:%D %R}", FmtLocalTime(time));
        HE_EXPECT_EQ(actual, "05/19/22 08:15");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, fmt_duration)
{
    constexpr Duration duration = 1_week + 2_day + 4_hour + 8_min + 16_sec + 32_ms + 64_us + 128_ns;

    // Default
    {
        String actual = Format("{}", duration);
        HE_EXPECT_EQ(actual, "792496032064128ns");
    }

    // All the specs
    {
        String actual = Format("{:%W %D %H %M %S %m %u %n %%}", duration);
        HE_EXPECT_EQ(actual, "1 2 4 8 16 32 64 128 %");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, literals)
{
    {
        constexpr Duration value = 1_ns;
        static_assert(value.val == 1);
        HE_EXPECT_EQ(value.val, 1);
    }

    {
        constexpr Duration value = 1_us;
        static_assert(value.val == 1000);
        HE_EXPECT_EQ(value.val, 1000);
    }

    {
        constexpr Duration value = 1_ms;
        static_assert(value.val == 1000000);
        HE_EXPECT_EQ(value.val, 1000000);
    }

    {
        constexpr Duration value = 1_sec;
        static_assert(value.val == 1000000000);
        HE_EXPECT_EQ(value.val, 1000000000);
    }

    {
        constexpr Duration value = 1_min;
        static_assert(value.val == 60000000000);
        HE_EXPECT_EQ(value.val, 60000000000);
    }

    {
        constexpr Duration value = 1_hour;
        static_assert(value.val == 3600000000000);
        HE_EXPECT_EQ(value.val, 3600000000000);
    }

    {
        constexpr Duration value = 1_day;
        static_assert(value.val == 86400000000000);
        HE_EXPECT_EQ(value.val, 86400000000000);
    }

    {
        constexpr Duration value = 1_week;
        static_assert(value.val == 604800000000000);
        HE_EXPECT_EQ(value.val, 604800000000000);
    }
}
