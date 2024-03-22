// Copyright Chad Engler

#include "he/core/clock_fmt.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/string.h"
#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/wstr.h"
#include "he/core/win32_min.h"

#include "fmt_private.h"

#include <time.h>

namespace he
{
    // --------------------------------------------------------------------------------------------
    template <typename T, typename = void>
    struct _TmHasGmtOff : FalseType {};

    template <typename T>
    struct _TmHasGmtOff<T, Void<decltype(T::tm_gmtoff)>> : TrueType {};

    template <typename T, typename = void>
    struct _TmHasZone : FalseType {};

    template <typename T>
    struct _TmHasZone<T, Void<decltype(T::tm_zone)>> : TrueType {};

    class _TimeSpecFormatter
    {
    public:
        _TimeSpecFormatter(String& out, const tm& t, const Duration& subsecs)
            : m_out(out)
            , m_tm(t)
            , m_subsecs(subsecs)
        {}

    public:
        void OnError([[maybe_unused]] const char* msg)
        {
            HE_ASSERT(false, HE_MSG(msg));
        }

        void OnText(const char* begin, const char* end)
        {
            m_out.Append(begin, end);
        }

        void OnYear([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltRep = writes year in the alternative representation, e.g.平成23年 (year Heisei 23) instead of 2011年 (year 2011) in ja_JP locale

            WriteYear(TmYear());
        }

        void OnShortYear([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltRep = writes year as offset from locale's alternative calendar period %EC (locale-dependent)
            // AltNum = writes last 2 digits of year using the alternative numeric system, e.g. 十一 instead of 11 in ja_JP locale

            Write2(SplitYearLower(TmYear()));
        }

        void OnCentury([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltRep = writes name of the base year (period) in the locale's alternative representation, e.g. 平成 (Heisei era) in ja_JP

            const int64_t year = TmYear();
            const int64_t upper = year / 100;
            if (year >= -99 && year < 0)
            {
                // Zero upper on negative year.
                m_out.PushBack('-');
                m_out.PushBack('0');
            }
            else if (upper >= 0 && upper < 100)
            {
                Write2(static_cast<int>(upper));
            }
            else
            {
                FormatTo(m_out, "{}", upper);
            }
        }

        void OnOffsetYear()
        {
            Write2(SplitYearLower(TmYear()));
        }

        void OnIsoWeekBasedYear()
        {
            WriteYear(TmIsoWeekYear());
        }

        void OnIsoWeekBasedShortYear()
        {
            Write2(SplitYearLower(TmIsoWeekYear()));
        }

        void OnAbbrWeekday()
        {
            m_out.Append(TmWDayShortName(TmWDay()));
        }

        void OnFullWeekday()
        {
            m_out.Append(TmWDayFullName(TmWDay()));
        }

        void OnDec0Weekday([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltNum = writes weekday, where Sunday is 0, using the alternative numeric system, e.g. 二 instead of 2 in ja_JP locale

            Write1(TmWDay());
        }

        void OnDec1Weekday([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltNum = writes weekday, where Monday is 1, using the alternative numeric system, e.g. 二 instead of 2 in ja_JP locale

            const int wday = TmWDay();
            Write1(wday == 0 ? DaysPerWeek : wday);
        }

        void OnAbbrMonth()
        {
            m_out.Append(TmMonShortName(TmMon()));
        }

        void OnFullMonth()
        {
            m_out.Append(TmMonFullName(TmMon()));
        }

        void OnDecMonth([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltNum = writes month using the alternative numeric system, e.g. 十二 instead of 12 in ja_JP locale

            Write2(TmMon() + 1);
        }

        void OnDec0WeekOfYear([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltNum = writes month using the alternative numeric system, e.g. 十二 instead of 12 in ja_JP locale

            Write2((TmYDay() + DaysPerWeek - TmWDay()) / DaysPerWeek);
        }

        void OnDec1WeekOfYear([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltNum = writes week of the year, as by %W, using the alternative numeric system, e.g. 五十二 instead of 52 in ja_JP locale

            const int wday = TmWDay();
            Write2((TmYDay() + DaysPerWeek - (wday == 0 ? (DaysPerWeek - 1) : (wday - 1))) / DaysPerWeek);
        }

        void OnIsoWeekOfYear([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltNum = writes week of the year, as by %V, using the alternative numeric system, e.g. 五十二 instead of 52 in ja_JP locale

            Write2(TmIsoWeekOfYear());
        }

        void OnDayOfYear()
        {
            const int yday = TmYDay() + 1;
            Write1(yday / 100);
            Write2(yday % 100);
        }

        void OnDayOfMonth([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltNum = writes zero-based day of the month using the alternative numeric system, e.g. 二十七 instead of 27 in ja_JP locale

            Write2(TmMDay());
        }

        void OnDayOfMonthSpace([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltNum = writes one-based day of the month using the alternative numeric system, e.g. 二十七 instead of 27 in ja_JP locale

            const uint32_t mday = static_cast<uint32_t>(TmMDay()) % 100;
            const char* d2 = Digits2(mday);
            m_out.PushBack(mday < 10 ? ' ' : d2[0]);
            m_out.PushBack(d2[1]);
        }

        void On24Hour([[maybe_unused]] _TimeSpecAlt alt, _TimeSpecPadding pad)
        {
            // Alt not supported:
            // AltNum = writes hour from 24-hour clock using the alternative numeric system, e.g. 十八 instead of 18 in ja_JP locale

            Write2(TmHour(), pad);
        }

        void On12Hour([[maybe_unused]] _TimeSpecAlt alt, _TimeSpecPadding pad)
        {
            // Alt not supported:
            // AltNum = writes hour from 12-hour clock using the alternative numeric system, e.g. 六 instead of 06 in ja_JP locale

            Write2(TmHour12(), pad);
        }

        void OnMinute([[maybe_unused]] _TimeSpecAlt alt, _TimeSpecPadding pad)
        {
            // Alt not supported:
            // AltNum = writes minute using the alternative numeric system, e.g. 二十五 instead of 25 in ja_JP locale

            Write2(TmMin(), pad);
        }

        void OnSecond([[maybe_unused]] _TimeSpecAlt alt, _TimeSpecPadding pad)
        {
            // Alt not supported:
            // AltNum = writes second using the alternative numeric system, e.g. 二十四 instead of 24 in ja_JP locale

            Write2(TmSec(), pad);

            if (m_subsecs == Duration_Zero)
                return;

            // Durations are nanoseconds, which is 9 orders of magnitude smaller than seconds.
            constexpr uint32_t FractionalDigitsCount = 9;

            const Duration wholeSecs = FromPeriod<Seconds>(ToPeriod<Seconds>(m_subsecs));
            const Duration fractionSecs = m_subsecs - wholeSecs;
            const uint64_t uvalue = static_cast<uint64_t>(fractionSecs.val);
            const uint32_t digitsCount = CountDigits(uvalue);

            const uint32_t leadingZeroes = Max(0u, FractionalDigitsCount - digitsCount);
            m_out.PushBack('.');

            if (leadingZeroes > 0)
            {
                m_out.Resize(m_out.Size() + leadingZeroes, '0');
            }

            char* it = FmtResize(m_out, digitsCount);
            FormatDecimal(it, uvalue, digitsCount);
        }

        void OnDatetime([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltRep = writes alternative date and time string, e.g. using 平成23年 (year Heisei 23) instead of 2011年 (year 2011) in ja_JP locale

            OnAbbrWeekday();
            m_out.PushBack(' ');
            OnAbbrMonth();
            m_out.PushBack(' ');
            OnDayOfMonthSpace(_TimeSpecAlt::None);
            m_out.PushBack(' ');
            OnIsoTime();
            m_out.PushBack(' ');
            OnYear(_TimeSpecAlt::None);
        }

        void OnLocDate([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltRep = writes alternative date representation, e.g. using 平成23年 (year Heisei 23) instead of 2011年 (year 2011) in ja_JP locale

            OnUsDate();
        }

        void OnLocTime([[maybe_unused]] _TimeSpecAlt alt)
        {
            // Alt not supported:
            // AltRep = writes alternative time representation (locale dependent)

            OnIsoTime();
        }

        void OnUsDate()
        {
            char buf[8];
            const uint32_t mon = static_cast<uint32_t>(TmMon() + 1);
            const uint32_t mday = static_cast<uint32_t>(TmMDay());
            const uint32_t year = static_cast<uint32_t>(SplitYearLower(TmYear()));
            WriteDigit2Separated(buf, mon, mday, year, '/');
            m_out.Append(buf, buf + HE_LENGTH_OF(buf));
        }

        void OnIsoDate()
        {
            int64_t year = TmYear();
            char buf[10];
            size_t offset = 0;
            if (year >= 0 && year < 10000)
            {
                MemCopy(buf, Digits2(static_cast<size_t>(year / 100)), 2);
            }
            else
            {
                offset = 4;
                WriteYearExtended(year);
                year = 0;
            }

            const uint32_t uyear = static_cast<uint32_t>(year % 100);
            const uint32_t umon = static_cast<uint32_t>(TmMon() + 1);
            const uint32_t umday = static_cast<uint32_t>(TmMDay());
            WriteDigit2Separated(buf + 2, uyear, umon, umday, '-');
            m_out.Append(buf + offset, buf + HE_LENGTH_OF(buf));
        }

        void On12HourTime()
        {
            char buf[8];
            const uint32_t hour12 = static_cast<uint32_t>(TmHour12());
            const uint32_t min = static_cast<uint32_t>(TmMin());
            const uint32_t sec = static_cast<uint32_t>(TmSec());
            WriteDigit2Separated(buf, hour12, min, sec, ':');
            m_out.Append(buf, buf + HE_LENGTH_OF(buf));
            m_out.PushBack(' ');
            OnAmPm();
        }

        void On24HourTime()
        {
            Write2(TmHour());
            m_out.PushBack(':');
            Write2(TmMin());
        }

        void OnIsoTime()
        {
            On24HourTime();
            m_out.PushBack(':');
            OnSecond(_TimeSpecAlt::None, _TimeSpecPadding::Unknown);
        }

        void OnAmPm()
        {
            m_out.PushBack(TmHour() < 12 ? 'A' : 'P');
            m_out.PushBack('M');
        }

        void OnUtcOffset(_TimeSpecAlt alt)
        {
            FormatUtcOffsetImpl(m_tm, alt);
        }

        void OnTzName()
        {
            FormatTzNameImpl(m_tm);
        }

    private:
        static constexpr int DaysPerWeek = 7;

        int TmSec() const
        {
            HE_ASSERT(m_tm.tm_sec >= 0 && m_tm.tm_sec <= 61, HE_KV(tm_sec, m_tm.tm_sec));
            return m_tm.tm_sec;
        }

        int TmMin() const
        {
            HE_ASSERT(m_tm.tm_min >= 0 && m_tm.tm_min <= 59, HE_KV(tm_min, m_tm.tm_min));
            return m_tm.tm_min;
        }

        int TmHour() const
        {
            HE_ASSERT(m_tm.tm_hour >= 0 && m_tm.tm_hour <= 23, HE_KV(tm_hour, m_tm.tm_hour));
            return m_tm.tm_hour;
        }

        int TmMDay() const
        {
            HE_ASSERT(m_tm.tm_mday >= 1 && m_tm.tm_mday <= 31, HE_KV(tm_mday, m_tm.tm_mday));
            return m_tm.tm_mday;
        }

        int TmMon() const
        {
            HE_ASSERT(m_tm.tm_mon >= 0 && m_tm.tm_mon <= 11, HE_KV(tm_mon, m_tm.tm_mon));
            return m_tm.tm_mon;
        }

        int64_t TmYear() const
        {
            return 1900ll + m_tm.tm_year;
        }

        int TmWDay() const
        {
            HE_ASSERT(m_tm.tm_wday >= 0 && m_tm.tm_wday <= 6, HE_KV(tm_wday, m_tm.tm_wday));
            return m_tm.tm_wday;
        }

        int TmYDay() const
        {
            HE_ASSERT(m_tm.tm_yday >= 0 && m_tm.tm_yday <= 365, HE_KV(tm_yday, m_tm.tm_yday));
            return m_tm.tm_yday;
        }

        int TmHour12() const
        {
            const int h = TmHour();
            const int z = h < 12 ? h : h - 12;
            return z == 0 ? 12 : z;
        }

        int64_t TmIsoWeekYear() const
        {
            const int64_t year = TmYear();
            const int w = IsoWeekNum(TmYDay(), TmWDay());

            if (w < 1)
                return year - 1;

            if (w > IsoYearWeeks(year))
                return year + 1;

            return year;
        }

        int TmIsoWeekOfYear() const
        {
            const int64_t year = TmYear();
            const int w = IsoWeekNum(TmYDay(), TmWDay());

            if (w < 1)
                return IsoYearWeeks(year - 1);

            if (w > IsoYearWeeks(year))
                return 1;

            return w;
        }

        // POSIX and the C Standard are unclear or inconsistent about what %C and %y
        // do if the year is negative or exceeds 9999. Use the convention that %C
        // concatenated with %y yields the same output as %Y, and that %Y contains at
        // least 4 characters, with more only if necessary.
        static constexpr int SplitYearLower(int64_t year)
        {
            int64_t l = year % 100;
            if (l < 0)
                l = -l;
            return static_cast<int>(l);
        }

        // Algorithm: https://en.wikipedia.org/wiki/ISO_week_date.
        static constexpr int IsoYearWeeks(int64_t currentYear)
        {
            const int64_t prevYear = currentYear - 1;
            const int64_t currP = (currentYear + currentYear / 4 - currentYear / 100 + currentYear / 400) % DaysPerWeek;
            const int64_t prevP = (prevYear + prevYear / 4 - prevYear / 100 + prevYear / 400) % DaysPerWeek;
            return 52 + ((currP == 4 || prevP == 3) ? 1 : 0);
        }

        static constexpr int IsoWeekNum(int yday, int wday)
        {
            return (yday + 11 - (wday == 0 ? DaysPerWeek : wday)) / DaysPerWeek;
        }

        static const char* TmMonFullName(int mon)
        {
            static constexpr const char* Names[] =
            {
                "January", "February", "March", "April", "May", "June",
                "July", "August", "September", "October", "November", "December"
            };
            return mon >= 0 && mon <= 11 ? Names[mon] : "?";
        }

        static const char* TmMonShortName(int mon)
        {
            static constexpr const char* Names[] =
            {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
            };
            return mon >= 0 && mon <= 11 ? Names[mon] : "???";
        }

        static const char* TmWDayFullName(int wday)
        {
            static constexpr const char* Names[] = { "Sunday",   "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
            return wday >= 0 && wday <= 6 ? Names[wday] : "?";
        }

        static const char* TmWDayShortName(int wday)
        {
            static constexpr const char* Names[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
            return wday >= 0 && wday <= 6 ? Names[wday] : "???";
        }

    private:
        void Write1(int value)
        {
            const uint32_t uvalue = static_cast<uint32_t>(value);
            m_out.PushBack(static_cast<char>('0' + (uvalue % 10)));
        }

        void Write2(int value)
        {
            const uint32_t uvalue = static_cast<uint32_t>(value);
            const char* d = Digits2(uvalue % 100);
            m_out.PushBack(*d++);
            m_out.PushBack(*d);
        }

        void Write2(int value, _TimeSpecPadding pad)
        {
            uint32_t v = static_cast<uint32_t>(value) % 100;
            if (v >= 10)
            {
                const char* d = Digits2(v);
                m_out.PushBack(*d++);
                m_out.PushBack(*d);
            }
            else
            {
                WritePadding(pad);
                m_out.PushBack(static_cast<char>('0' + v));
            }
        }

        void WritePadding(_TimeSpecPadding pad, uint32_t width)
        {
            if (pad == _TimeSpecPadding::None)
                return;

            const char ch = pad == _TimeSpecPadding::Space ? ' ' : '0';
            m_out.Resize(m_out.Size() + width, ch);
        }

        void WritePadding(_TimeSpecPadding pad)
        {
            if (pad != _TimeSpecPadding::None)
            {
                m_out.PushBack(pad == _TimeSpecPadding::Space ? ' ' : '0');
            }
        }

        void WriteYearExtended(int64_t year)
        {
            // At least 4 characters.
            int32_t width = 4;
            if (year < 0)
            {
                m_out.PushBack('-');
                year = 0 - year;
                --width;
            }
            const uint64_t n = static_cast<uint64_t>(year);
            const int32_t digitsCount = CountDigits(n);
            if (width > digitsCount)
            {
                m_out.Resize(m_out.Size() + (width - digitsCount), '0');
            }
            char* it = FmtResize(m_out, digitsCount);
            FormatDecimal(it, n, digitsCount);
        }

        void WriteYear(int64_t year)
        {
            if (year >= 0 && year < 10000)
            {
                Write2(static_cast<int>(year / 100));
                Write2(static_cast<int>(year % 100));
            }
            else
            {
                WriteYearExtended(year);
            }
        }

        void WriteUtcOffset(long offset, _TimeSpecAlt alt)
        {
            if (offset < 0)
            {
                m_out.PushBack('-');
                offset = -offset;
            }
            else
            {
                m_out.PushBack('+');
            }

            offset /= 60;
            Write2(static_cast<int32_t>(offset / 60));

            if (alt != _TimeSpecAlt::None)
                m_out.PushBack(':');

            Write2(static_cast<int32_t>(offset % 60));
        }

        // Writes two-digit numbers a, b and c separated by sep to buf.
        // The method by Pavel Novikov based on
        // https://johnnylee-sde.github.io/Fast-unsigned-integer-to-time-string/.
        void WriteDigit2Separated(char buf[8], uint32_t a, uint32_t b, uint32_t c, char sep)
        {
            uint64_t digits = a | (b << 24) | (static_cast<uint64_t>(c) << 48);

            // Convert each value to BCD.
            // We have x = a * 10 + b and we want to convert it to BCD y = a * 16 + b.
            // The difference is
            //   y - x = a * 6
            // a can be found from x:
            //   a = floor(x / 10)
            // then
            //   y = x + a * 6 = x + floor(x / 10) * 6
            // floor(x / 10) is (x * 205) >> 11 (needs 16 bits).
            digits += (((digits * 205) >> 11) & 0x000f00000f00000f) * 6;
            // Put low nibbles to high bytes and high nibbles to low bytes.
            digits = ((digits & 0x00f00000f00000f0) >> 4) | ((digits & 0x000f00000f00000f) << 8);
            const uint64_t usep = static_cast<uint64_t>(sep);
            // Add ASCII '0' to each digit byte and insert separators.
            digits |= 0x3030003030003030 | (usep << 16) | (usep << 40);

            constexpr const size_t DigitsLength = sizeof(uint64_t);
        #if HE_CPU_BIG_ENDIAN
            char tmp[DigitsLength];
            MemCopy(tmp, &digits, DigitsLength);

            for (size_t i = 0; i < DigitsLength; ++i)
            {
                buf[i] = tmp[(DigitsLength - 1) - i];
            }
        #else
            MemCopy(buf, &digits, DigitsLength);
        #endif
        }

        template <typename T> requires(_TmHasGmtOff<T>::Value)
        void FormatUtcOffsetImpl(const T& t, _TimeSpecAlt alt)
        {
            WriteUtcOffset(t.tm_gmtoff, alt);
        }

        template <typename T> requires(!_TmHasGmtOff<T>::Value)
        void FormatUtcOffsetImpl(const T& t, _TimeSpecAlt alt)
        {
        #if defined(HE_PLATFORM_API_WIN32) && defined(_UCRT)
        #if (!defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
            [[maybe_unused]] static bool s_init = []() -> bool { _tzset(); return true; }();
        #endif
            long offset = 0;
            _get_timezone(&offset);
            if (t.tm_isdst)
            {
                long dstbias = 0;
                _get_dstbias(&dstbias);
                offset += dstbias;
            }
            WriteUtcOffset(-offset, alt);
        #else
            // Extract timezone offset from timezone conversion functions.
            struct tm gtm = t;
            const time_t gt = mktime(&gtm);

            struct tm ltm{};
            gmtime_r(&gt, &ltm);

            const time_t lt = mktime(&ltm);
            const long offset = gt - lt;
            WriteUtcOffset(offset, alt);
        #endif
        }

        template <typename T> requires(_TmHasZone<T>::Value)
        void FormatTzNameImpl(const T& t)
        {
            m_out.Append(t.tm_zone);
        }

        template <typename T> requires(!_TmHasZone<T>::Value)
        void FormatTzNameImpl(const T&)
        {
        #if defined(HE_PLATFORM_API_WIN32)
            TIME_ZONE_INFORMATION tzInfo{};
            const DWORD rc = ::GetTimeZoneInformation(&tzInfo);

            if (rc == TIME_ZONE_ID_DAYLIGHT)
                m_out.Append(HE_TO_MBSTR(tzInfo.DaylightName));
            else if (rc == TIME_ZONE_ID_STANDARD)
                m_out.Append(HE_TO_MBSTR(tzInfo.StandardName));
        #else
            // Nothing to do here. Do we actually hit this case on any platform?
            static_assert(_TmHasZone<T>::Value, "No timezone name available.");
        #endif
        }

    private:
        String& m_out;
        const tm& m_tm;
        const Duration& m_subsecs;
    };

    void Formatter<FmtLocalTime>::Format(String& out, const FmtLocalTime& t) const
    {
        const time_t time = t.time.val / Seconds::Ratio;
        struct tm tm{};
    #if HE_COMPILER_MSVC
        localtime_s(&tm, &time);
    #else
        localtime_r(&time, &tm);
    #endif

        // TODO: subsecs
        _TimeSpecFormatter formatter(out, tm, Duration_Zero);
        _ParseTimeSpecFormat(spec.Begin(), spec.End(), formatter);
    }

    void Formatter<SystemTime>::Format(String& out, const SystemTime& t) const
    {
        const time_t time = t.val / Seconds::Ratio;
        struct tm tm{};
    #if HE_COMPILER_MSVC
        gmtime_s(&tm, &time);
    #else
        gmtime_r(&time, &tm);
    #endif

        // TODO: subsecs
        _TimeSpecFormatter formatter(out, tm, Duration_Zero);
        _ParseTimeSpecFormat(spec.Begin(), spec.End(), formatter);
    }

    // --------------------------------------------------------------------------------------------
    template <typename T>
    static void _WritePeriod(String& out, Duration& duration)
    {
        const int64_t count = ToPeriod<T>(duration);
        duration.val -= FromPeriod<T>(count).val;
        FormatTo(out, "{}", count);
    }

    void Formatter<Duration>::Format(String& out, const Duration& d) const
    {
        if (type == 'i')
        {
            int64_t value = 0;
            switch (period)
            {
                case Period::Nanoseconds: value = ToPeriod<Nanoseconds>(d); break;
                case Period::Microseconds: value = ToPeriod<Microseconds>(d); break;
                case Period::Milliseconds: value = ToPeriod<Milliseconds>(d); break;
                case Period::Seconds: value = ToPeriod<Seconds>(d); break;
                case Period::Minutes: value = ToPeriod<Minutes>(d); break;
                case Period::Hours: value = ToPeriod<Hours>(d); break;
                case Period::Days: value = ToPeriod<Days>(d); break;
                case Period::Weeks: value = ToPeriod<Weeks>(d); break;
                default: break;
            }
            FormatTo(out, "{}", value);
        }
        else
        {
            double value = 0.0;
            switch (period)
            {
                case Period::Nanoseconds: value = ToPeriod<Nanoseconds, double>(d); break;
                case Period::Microseconds: value = ToPeriod<Microseconds, double>(d); break;
                case Period::Milliseconds: value = ToPeriod<Milliseconds, double>(d); break;
                case Period::Seconds: value = ToPeriod<Seconds, double>(d); break;
                case Period::Minutes: value = ToPeriod<Minutes, double>(d); break;
                case Period::Hours: value = ToPeriod<Hours, double>(d); break;
                case Period::Days: value = ToPeriod<Days, double>(d); break;
                case Period::Weeks: value = ToPeriod<Weeks, double>(d); break;
                default: break;
            }

            // TODO: If I could use the internal fmt functions I could do more robust formatting here.
            String spec = "{:";
            if (precision >= 0)
            {
                FormatTo(spec, ".{}", precision);
            }
            spec.PushBack(type);
            spec.Append("}");
            FormatTo(out, FmtRuntime(spec), value);
        }

        if (suffix)
        {
            switch (period)
            {
                case Period::Nanoseconds: out.Append("ns"); break;
                case Period::Microseconds: out.Append("µs"); break;
                case Period::Milliseconds: out.Append("ms"); break;
                case Period::Seconds: out.Append("s"); break;
                case Period::Minutes: out.Append("m"); break;
                case Period::Hours: out.Append("h"); break;
                case Period::Days: out.Append("D"); break;
                case Period::Weeks: out.Append("W"); break;
                case Period::Months: out.Append("M"); break;
                case Period::Years: out.Append("Y"); break;
                default: break;
            }
        }
    }
}
