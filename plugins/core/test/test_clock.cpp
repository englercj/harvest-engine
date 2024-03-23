// Copyright Chad Engler

#include "he/core/clock.h"
#include "he/core/clock_fmt.h"

#include "he/core/fmt.h"
#include "he/core/type_traits.h"
#include "he/core/test.h"

#include <iostream>

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
    static_assert(IsNearlyEqualULP(ToPeriod<Milliseconds, double>(Duration{ 2000 }), 0.002, 1));
    static_assert(IsNearlyEqualULP(ToPeriod<Seconds, double>(Duration{ 2000 }), 0.000002, 1));

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
    static_assert(IsNearlyEqualULP(ToPeriod<Milliseconds, float>(Duration{ 2000 }), 0.002f, 1));
    static_assert(IsNearlyEqualULP(ToPeriod<Seconds, float>(Duration{ 2000 }), 0.000002f, 1));

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
    std::cout << "    Local Timezone Offset = " << ToPeriod<Hours, float>(tzOffset) << std::endl;
}
#include "he/core/debugger.h"
// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, fmt_time)
{
    // 8:15 AM, Thursday, May 19, 2022 (PDT)
    constexpr SystemTime time{ 1652973325730225600 };

    // 10:03 AM, Sunday, Feb 25, 2024 (PST)
    constexpr SystemTime time2{ 1708884218444204000 };

    // Special characters
    HE_EXPECT_EQ(Format("{:%%}", time), "%");
    HE_EXPECT_EQ(Format("{:%n}", time), "\n");
    HE_EXPECT_EQ(Format("{:%t}", time), "\t");

    // Year
    HE_EXPECT_EQ(Format("{:%Y}", time), "2022");
    HE_EXPECT_EQ(Format("{:%y}", time), "22");
    HE_EXPECT_EQ(Format("{:%C}", time), "20");
    HE_EXPECT_EQ(Format("{:%G}", time), "2022");
    HE_EXPECT_EQ(Format("{:%g}", time), "22");
    HE_EXPECT_EQ(Format("{:%C%y}", time), "2022");

    // Day of the week
    HE_EXPECT_EQ(Format("{:%a}", time), "Thu");
    HE_EXPECT_EQ(Format("{:%A}", time), "Thursday");
    HE_EXPECT_EQ(Format("{:%w}", time), "4");
    HE_EXPECT_EQ(Format("{:%u}", time), "4");

    HE_EXPECT_EQ(Format("{:%a}", time2), "Sun");
    HE_EXPECT_EQ(Format("{:%A}", time2), "Sunday");
    HE_EXPECT_EQ(Format("{:%w}", time2), "0");
    HE_EXPECT_EQ(Format("{:%u}", time2), "7");

    // Month
    HE_EXPECT_EQ(Format("{:%h}", time), "May");
    HE_EXPECT_EQ(Format("{:%b}", time), "May");
    HE_EXPECT_EQ(Format("{:%B}", time), "May");
    HE_EXPECT_EQ(Format("{:%m}", time), "05");

    HE_EXPECT_EQ(Format("{:%h}", time2), "Feb");
    HE_EXPECT_EQ(Format("{:%b}", time2), "Feb");
    HE_EXPECT_EQ(Format("{:%B}", time2), "February");
    HE_EXPECT_EQ(Format("{:%m}", time2), "02");

    // Day of the year/month
    HE_EXPECT_EQ(Format("{:%U}", time), "20");
    HE_EXPECT_EQ(Format("{:%W}", time), "20");
    HE_EXPECT_EQ(Format("{:%V}", time), "20");
    HE_EXPECT_EQ(Format("{:%j}", time), "139");
    HE_EXPECT_EQ(Format("{:%d}", time), "19");
    HE_EXPECT_EQ(Format("{:%e}", time), "19");

    HE_EXPECT_EQ(Format("{:%U}", time2), "08");
    HE_EXPECT_EQ(Format("{:%W}", time2), "08");
    HE_EXPECT_EQ(Format("{:%V}", time2), "08");
    HE_EXPECT_EQ(Format("{:%j}", time2), "056");
    HE_EXPECT_EQ(Format("{:%d}", time2), "25");
    HE_EXPECT_EQ(Format("{:%e}", time2), "25");

    // Hour, minute, second
    HE_EXPECT_EQ(Format("{:%H}", time), "15");
    HE_EXPECT_EQ(Format("{:%I}", time), "03");
    HE_EXPECT_EQ(Format("{:%M}", time), "15");
    HE_EXPECT_EQ(Format("{:%S}", time), "25");

    HE_EXPECT_EQ(Format("{:%H}", time2), "18");
    HE_EXPECT_EQ(Format("{:%I}", time2), "06");
    HE_EXPECT_EQ(Format("{:%M}", time2), "03");
    HE_EXPECT_EQ(Format("{:%S}", time2), "38");

    // Other
    HE_EXPECT_EQ(Format("{:%c}", time), "Thu May 19 15:15:25 2022");
    HE_EXPECT_EQ(Format("{:%x}", time), "05/19/22");
    HE_EXPECT_EQ(Format("{:%X}", time), "15:15:25");
    HE_EXPECT_EQ(Format("{:%D}", time), "05/19/22");
    HE_EXPECT_EQ(Format("{:%F}", time), "2022-05-19");
    HE_EXPECT_EQ(Format("{:%r}", time), "03:15:25 PM");
    HE_EXPECT_EQ(Format("{:%R}", time), "15:15");
    HE_EXPECT_EQ(Format("{:%T}", time), "15:15:25");
    HE_EXPECT_EQ(Format("{:%p}", time), "PM");
    HE_EXPECT_EQ(Format("{:%z}", time), "-0800");
    //HE_EXPECT_EQ(Format("{:%Z}", time), "Pacific Standard Time");

    // Alternative representations
    HE_EXPECT_EQ(Format("{:%EY}", time), "2022");
    HE_EXPECT_EQ(Format("{:%Ey}", time), "22");
    HE_EXPECT_EQ(Format("{:%EC}", time), "20");
    HE_EXPECT_EQ(Format("{:%Ec}", time), "Thu May 19 15:15:25 2022");
    HE_EXPECT_EQ(Format("{:%Ex}", time), "05/19/22");
    HE_EXPECT_EQ(Format("{:%EX}", time), "15:15:25");
    HE_EXPECT_EQ(Format("{:%Ez}", time), "-08:00");

    // Alternative numeric systems
    HE_EXPECT_EQ(Format("{:%Oy}", time), "22");
    HE_EXPECT_EQ(Format("{:%Om}", time), "05");
    HE_EXPECT_EQ(Format("{:%OU}", time), "20");
    HE_EXPECT_EQ(Format("{:%OW}", time), "20");
    HE_EXPECT_EQ(Format("{:%OV}", time), "20");
    HE_EXPECT_EQ(Format("{:%Od}", time), "19");
    HE_EXPECT_EQ(Format("{:%Oe}", time), "19");
    HE_EXPECT_EQ(Format("{:%Ow}", time), "4");
    HE_EXPECT_EQ(Format("{:%Ou}", time), "4");
    HE_EXPECT_EQ(Format("{:%OH}", time), "15");
    HE_EXPECT_EQ(Format("{:%OI}", time), "03");
    HE_EXPECT_EQ(Format("{:%OM}", time), "15");
    HE_EXPECT_EQ(Format("{:%OS}", time), "25");
    HE_EXPECT_EQ(Format("{:%Oz}", time), "-08:00");

    // Default format
    HE_EXPECT_EQ(Format("{}", time), "2022-05-19T15:15:25Z");
    HE_EXPECT_EQ(Format("{}", FmtUtcTime(time)), "2022-05-19T15:15:25Z");
    HE_EXPECT_EQ(Format("{}", FmtLocalTime(time)), "2022-05-19T08:15:25-0700");

    HE_EXPECT_EQ(Format("{}", time2), "2024-02-25T18:03:38Z");
    HE_EXPECT_EQ(Format("{}", FmtUtcTime(time2)), "2024-02-25T18:03:38Z");
    HE_EXPECT_EQ(Format("{}", FmtLocalTime(time2)), "2024-02-25T10:03:38-0800");

    // Custom format
    HE_EXPECT_EQ(Format("{:%A %b %d, %y %T %p}", time), "Thursday May 19, 22 15:15:25 PM");
    HE_EXPECT_EQ(Format("{:%A %b %d, %y %T %p}", FmtUtcTime(time)), "Thursday May 19, 22 15:15:25 PM");
    HE_EXPECT_EQ(Format("{:%D %R}", FmtLocalTime(time)), "05/19/22 08:15");

    HE_EXPECT_EQ(Format("{:%A %b %d, %y %T %p}", time2), "Sunday Feb 25, 24 18:03:38 PM");
    HE_EXPECT_EQ(Format("{:%A %b %d, %y %T %p}", FmtUtcTime(time2)), "Sunday Feb 25, 24 18:03:38 PM");
    HE_EXPECT_EQ(Format("{:%D %R}", FmtLocalTime(time2)), "02/25/24 10:03");

    // With extra non-format text
    HE_EXPECT_EQ(Format("The date is: {:%Y-%m-%d %H:%M:%S}.", time), "The date is: 2022-05-19 15:15:25.");

    // Short year
    constexpr SystemTime shortYearTime{ 500ull * Years::Ratio };
    HE_EXPECT_EQ(Format("{}", shortYearTime), "2469-12-31T06:00:00Z");
    HE_EXPECT_EQ(Format("{:%Y}", shortYearTime), "2469");
    HE_EXPECT_EQ(Format("{:%y}", shortYearTime), "69");
    HE_EXPECT_EQ(Format("{:%C}", shortYearTime), "24");
    HE_EXPECT_EQ(Format("{:%G}", shortYearTime), "2470");
    HE_EXPECT_EQ(Format("{:%g}", shortYearTime), "70");
    HE_EXPECT_EQ(Format("{:%C%y}", shortYearTime), "2469");

    constexpr SystemTime shortYearTime2{ 27ull * Years::Ratio };
    HE_EXPECT_EQ(Format("{}", shortYearTime2), "1996-12-31T13:08:24Z");
    HE_EXPECT_EQ(Format("{:%Y}", shortYearTime2), "1996");
    HE_EXPECT_EQ(Format("{:%y}", shortYearTime2), "96");
    HE_EXPECT_EQ(Format("{:%C}", shortYearTime2), "19");
    HE_EXPECT_EQ(Format("{:%G}", shortYearTime2), "1997");
    HE_EXPECT_EQ(Format("{:%g}", shortYearTime2), "97");
    HE_EXPECT_EQ(Format("{:%C%y}", shortYearTime2), "1996");

    // Epoch
    constexpr SystemTime epochTime{ 0 };
    constexpr SystemTime endTimes{ Limits<uint64_t>::Max };
    HE_EXPECT_EQ(Format("{}", epochTime), "1970-01-01T00:00:00Z");
    HE_EXPECT_EQ(Format("{}", endTimes), "2554-07-21T23:34:33Z");

    // TODO: These are good tests for a DateTime / TimeSpan class I have yet to write.

    //// Far future
    //constexpr SystemTime farFutureTime{
    //    (10445ull * Years::Ratio)
    //    + FromPeriod<Months>(3).val
    //    + FromPeriod<Days>(25).val
    //    + FromPeriod<Hours>(11).val
    //    + FromPeriod<Minutes>(22).val
    //    + FromPeriod<Seconds>(33).val
    //};
    //HE_EXPECT_EQ(Format("The date is {:%Y-%m-%d %H:%M:%S}.", farFutureTime), "The date is 12345-04-25 11:22:33.");
    //HE_EXPECT_EQ(Format("{:%Y}", farFutureTime), "12345");
    //HE_EXPECT_EQ(Format("{:%C}", farFutureTime), "123");
    //HE_EXPECT_EQ(Format("{:%C%y}", farFutureTime), Format("{:%Y}", farFutureTime));
    //HE_EXPECT_EQ(Format("{:%D}", farFutureTime), "04/25/45");
    //HE_EXPECT_EQ(Format("{:%F}", farFutureTime), "12345-04-25");
    //HE_EXPECT_EQ(Format("{:%T}", farFutureTime), "11:22:33");

    //// Far past
    //constexpr SystemTime farPastTime{
    //    FromPeriod<Years>(-2001).val
    //    + FromPeriod<Months>(3).val
    //    + FromPeriod<Days>(25).val
    //    + FromPeriod<Hours>(11).val
    //    + FromPeriod<Minutes>(22).val
    //    + FromPeriod<Seconds>(33).val
    //};
    //HE_EXPECT_EQ(Format("The date is {:%Y-%m-%d %H:%M:%S}.", farPastTime), "The date is -101-04-25 11:22:33.");
    //HE_EXPECT_EQ(Format("{:%Y}", farPastTime), "-101");

    //// macOS  %C - "-1"
    //// Linux  %C - "-2"
    //// fmt    %C - "-1"
    //HE_EXPECT_EQ(Format("{:%C}", farPastTime), "-1");
    //HE_EXPECT_EQ(Format("{:%C%y}", farPastTime), Format("{:%Y}", farPastTime));

    //// macOS  %D - "04/25/01" (%y)
    //// Linux  %D - "04/25/99" (%y)
    //// fmt    %D - "04/25/01" (%y)
    //HE_EXPECT_EQ(Format("{:%D}", farPastTime), "04/25/01");

    //HE_EXPECT_EQ(Format("{:%F}", farPastTime), "-101-04-25");
    //HE_EXPECT_EQ(Format("{:%T}", farPastTime), "11:22:33");

    //constexpr SystemTime farPastTime2{
    //    FromPeriod<Years>(-1901).val
    //    + FromPeriod<Months>(3).val
    //    + FromPeriod<Days>(25).val
    //    + FromPeriod<Hours>(11).val
    //    + FromPeriod<Minutes>(22).val
    //    + FromPeriod<Seconds>(33).val
    //};
    //HE_EXPECT_EQ(Format("{:%Y}", farPastTime2), "-001");
    //HE_EXPECT_EQ(Format("{:%C%y}", farPastTime2), Format("{:%Y}", farPastTime2));

    //constexpr SystemTime farPastTime3{
    //    FromPeriod<Years>(-1911).val
    //    + FromPeriod<Months>(3).val
    //    + FromPeriod<Days>(25).val
    //    + FromPeriod<Hours>(11).val
    //    + FromPeriod<Minutes>(22).val
    //    + FromPeriod<Seconds>(33).val
    //};
    //HE_EXPECT_EQ(Format("{:%Y}", farPastTime3), "-011");
    //HE_EXPECT_EQ(Format("{:%C%y}", farPastTime3), Format("{:%Y}", farPastTime3));

    // Long output string
    {
        constexpr const char Expected[] = "Thu May 19 15:15:25 2022";
        constexpr uint32_t ExpectedLen = sizeof(Expected) - 1;

        const String buf = Format("{:%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c}", time);
        HE_EXPECT_EQ((buf.Size() % ExpectedLen), 0);

        const char* begin = buf.Begin();
        const char* end = buf.End();
        while (begin < end)
        {
            HE_EXPECT_EQ_MEM(begin, Expected, ExpectedLen);
            begin += ExpectedLen;
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, clock, fmt_duration)
{
    constexpr Duration duration = 1_week + 2_day + 4_hour + 8_min + 16_sec + 32_ms + 64_us + 128_ns;

    // Default
    HE_EXPECT_EQ(Format("{}", duration), "792496032064128ns");

    // Periods
    HE_EXPECT_EQ(Format("{:n}", duration), "792496032064128ns");
    HE_EXPECT_EQ(Format("{:u}", duration), "792496032064µs");
    HE_EXPECT_EQ(Format("{:m}", duration), "792496032ms");
    HE_EXPECT_EQ(Format("{:S}", duration), "792496s");
    HE_EXPECT_EQ(Format("{:M}", duration), "13208m");
    HE_EXPECT_EQ(Format("{:H}", duration), "220h");
    HE_EXPECT_EQ(Format("{:d}", duration), "9D");
    HE_EXPECT_EQ(Format("{:w}", duration), "1W");
    HE_EXPECT_EQ(Format("{:o}", duration), "0M");
    HE_EXPECT_EQ(Format("{:y}", duration), "0Y");

    // No Suffix
    HE_EXPECT_EQ(Format("{:#n}", duration), "792496032064128");
    HE_EXPECT_EQ(Format("{:#u}", duration), "792496032064");
    HE_EXPECT_EQ(Format("{:#m}", duration), "792496032");
    HE_EXPECT_EQ(Format("{:#S}", duration), "792496");
    HE_EXPECT_EQ(Format("{:#M}", duration), "13208");
    HE_EXPECT_EQ(Format("{:#H}", duration), "220");
    HE_EXPECT_EQ(Format("{:#d}", duration), "9");
    HE_EXPECT_EQ(Format("{:#w}", duration), "1");
    HE_EXPECT_EQ(Format("{:#o}", duration), "0");
    HE_EXPECT_EQ(Format("{:#y}", duration), "0");

    // Fixed Precision
    HE_EXPECT_EQ(Format("{:#nf.4}", duration), "792496032064128.0000");
    HE_EXPECT_EQ(Format("{:#uf.4}", duration), "792496032064.1280");
    HE_EXPECT_EQ(Format("{:#mf.4}", duration), "792496032.0641");
    HE_EXPECT_EQ(Format("{:#Sf.4}", duration), "792496.0320");
    HE_EXPECT_EQ(Format("{:#Mf.4}", duration), "13208.2672");
    HE_EXPECT_EQ(Format("{:#Hf.4}", duration), "220.1378");
    HE_EXPECT_EQ(Format("{:#df.4}", duration), "9.1724");
    HE_EXPECT_EQ(Format("{:#wf.4}", duration), "1.3103");
    HE_EXPECT_EQ(Format("{:#of.4}", duration), "0.0000");
    HE_EXPECT_EQ(Format("{:#yf.4}", duration), "0.0000");

    // General float format
    HE_EXPECT_EQ(Format("{:ng}", duration), "7.92496e+14ns");
    HE_EXPECT_EQ(Format("{:ug}", duration), "7.92496e+11µs");
    HE_EXPECT_EQ(Format("{:mg}", duration), "7.92496e+08ms");
    HE_EXPECT_EQ(Format("{:Sg}", duration), "792496s");
    HE_EXPECT_EQ(Format("{:Mg}", duration), "13208.3m");
    HE_EXPECT_EQ(Format("{:Hg}", duration), "220.138h");
    HE_EXPECT_EQ(Format("{:dg}", duration), "9.17241D");
    HE_EXPECT_EQ(Format("{:wg}", duration), "1.31034W");
    HE_EXPECT_EQ(Format("{:og}", duration), "0M");
    HE_EXPECT_EQ(Format("{:yg}", duration), "0Y");
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
