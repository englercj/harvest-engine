// Copyright Chad Engler

#include "he/core/clock.h"

#include "he/core/clock_fmt.h"
#include "he/core/test.h"
#include "he/core/type_traits.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, SystemClock)
{
    static_assert(std::is_same_v<SystemClock::Time, SystemTime>);
    SystemTime t = SystemClock::Now();
    HE_EXPECT_GT(t.ns, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, MonotonicClock)
{
    static_assert(std::is_same_v<MonotonicClock::Time, MonotonicTime>);
    MonotonicTime t = MonotonicClock::Now();
    HE_EXPECT_GT(t.ns, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, Duration)
{
    static_assert(Duration_Zero.ns == 0);
    static_assert(Duration_Max.ns == INT64_MAX);
    static_assert(Duration_Min.ns == INT64_MIN);

    HE_EXPECT_EQ(Duration_Zero.ns, 0);
    HE_EXPECT_EQ(Duration_Max.ns, INT64_MAX);
    HE_EXPECT_EQ(Duration_Min.ns, INT64_MIN);

    HE_EXPECT_EQ(Duration{ 15 }.ns, 15);
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
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, ToPeriod)
{
    static_assert(ToPeriod<Nanoseconds>(Duration_Zero) == 0);
    static_assert(ToPeriod<Microseconds>(Duration_Zero) == 0);
    static_assert(ToPeriod<Milliseconds>(Duration_Zero) == 0);
    static_assert(ToPeriod<Seconds>(Duration_Zero) == 0);
    static_assert(ToPeriod<Minutes>(Duration_Zero) == 0);
    static_assert(ToPeriod<Hours>(Duration_Zero) == 0);

    static_assert(ToPeriod<Nanoseconds>(Duration{ 2000 }) == 2000);
    static_assert(ToPeriod<Microseconds>(Duration{ 2000 }) == 2);
    static_assert(ToPeriod<Milliseconds>(Duration{ 2000 }) == 0);
    static_assert(ToPeriod<Seconds>(Duration{ 2000 }) == 0);
    static_assert(ToPeriod<Minutes>(Duration{ 2000 }) == 0);
    static_assert(ToPeriod<Hours>(Duration{ 2000 }) == 0);

    HE_EXPECT_EQ(ToPeriod<Nanoseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Microseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Milliseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Seconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Minutes>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToPeriod<Hours>(Duration_Zero), 0);

    HE_EXPECT_EQ(ToPeriod<Nanoseconds>(Duration{ 2000 }), 2000);
    HE_EXPECT_EQ(ToPeriod<Microseconds>(Duration{ 2000 }), 2);
    HE_EXPECT_EQ(ToPeriod<Milliseconds>(Duration{ 2000 }), 0);
    HE_EXPECT_EQ(ToPeriod<Seconds>(Duration{ 2000 }), 0);
    HE_EXPECT_EQ(ToPeriod<Minutes>(Duration{ 2000 }), 0);
    HE_EXPECT_EQ(ToPeriod<Hours>(Duration{ 2000 }), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, ToFloatPeriod)
{
    static_assert(ToFloatPeriod<Nanoseconds>(Duration_Zero) == 0);
    static_assert(ToFloatPeriod<Microseconds>(Duration_Zero) == 0);
    static_assert(ToFloatPeriod<Milliseconds>(Duration_Zero) == 0);
    static_assert(ToFloatPeriod<Seconds>(Duration_Zero) == 0);
    static_assert(ToFloatPeriod<Minutes>(Duration_Zero) == 0);
    static_assert(ToFloatPeriod<Hours>(Duration_Zero) == 0);

    static_assert(ToFloatPeriod<Nanoseconds>(Duration{ 2000 }) == 2000.0f);
    static_assert(ToFloatPeriod<Microseconds>(Duration{ 2000 }) == 2.0f);
    //static_assert(ToFloatPeriod<Milliseconds>(Duration{ 2000 }) == 0.002f);
    //static_assert(ToFloatPeriod<Seconds>(Duration{ 2000 }) == 0.000002f);

    HE_EXPECT_EQ(ToFloatPeriod<Nanoseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToFloatPeriod<Microseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToFloatPeriod<Milliseconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToFloatPeriod<Seconds>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToFloatPeriod<Minutes>(Duration_Zero), 0);
    HE_EXPECT_EQ(ToFloatPeriod<Hours>(Duration_Zero), 0);

    HE_EXPECT_EQ(ToFloatPeriod<Nanoseconds>(Duration{ 2000 }), 2000.0f);
    HE_EXPECT_EQ(ToFloatPeriod<Microseconds>(Duration{ 2000 }), 2.0f);
    HE_EXPECT_EQ(ToFloatPeriod<Milliseconds>(Duration{ 2000 }), 0.002f);
    HE_EXPECT_EQ(ToFloatPeriod<Seconds>(Duration{ 2000 }), 0.000002f);
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
    HE_EXPECT_EQ(a, T{ 690 });

    // T + D
    HE_EXPECT_EQ((t1 + d0), T{ 690 });

    // D + T
    HE_EXPECT_EQ((d0 + t1), T{ 690 });

    // D += D
    Duration b = d0;
    b += d1;
    HE_EXPECT_EQ(b, Duration{ 246 });

    // D + D
    HE_EXPECT_EQ((d0 + d1), Duration{ 246 });

    // T -= D
    T c = t0;
    c -= d0;
    HE_EXPECT_EQ(c, T{ 444 });

    // T - T
    HE_EXPECT_EQ((t1 - t0), Duration{ 0 });

    // T - D
    HE_EXPECT_EQ((t0 - d0), T{ 444 });

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
    HE_EXPECT_EQ(Win32FileTimeToSystemTime(t).ns, 0);
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
    HE_EXPECT_EQ(PosixTimeToSystemTime(posixTime).ns, 123456789987654321);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, PosixTimeFromSystemTime)
{
    SystemTime systemTime{ 123456789987654321 };
    auto test = PosixTimeFromSystemTime(systemTime);
    HE_EXPECT_EQ(test.tv_nsec, 987654321);
    HE_EXPECT_EQ(test.tv_sec, 123456789);
}
