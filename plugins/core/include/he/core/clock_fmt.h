// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/fmt.h"
#include "he/core/string_view.h"

namespace he
{
    class String;

    // --------------------------------------------------------------------------------------------

    /// Marks a \ref SystemTime to be formatted as a local time.
    struct FmtLocalTime
    {
        explicit FmtLocalTime(SystemTime t) : time(t) {}
        SystemTime time;
    };

    /// Marks a \ref SystemTime to be formatted as a UTC time.
    /// This is equivalent to formatting a \ref SystemTime directly, since UTC is the default.
    struct FmtUtcTime
    {
        explicit FmtUtcTime(SystemTime t) : time(t) {}
        SystemTime time;
    };

    // --------------------------------------------------------------------------------------------
    /// \internal
    enum class _TimeSpecPadding
    {
        Unknown,    ///< Padding is not specified.
        None,       ///< Do not pad the value.
        Space,      ///< Pad the value with spaces.
        Zero,       ///< Pad the value with zeros.
    };

    /// \internal
    enum class _TimeSpecAlt
    {
        None,       ///< The default representation.
        AltRep,     ///< Use a locale-dependant alternative representation.
        AltNum,     ///< Use a locale-dependant alternative numeric system.
    };

    // --------------------------------------------------------------------------------------------
    /// \internal
    template <typename Handler>
    constexpr const char* _ParseTimeSpecFormat(const char* begin, const char* end, Handler&& handler)
    {
        if (begin == end || *begin == '}')
            return begin;

        if (*begin != '%')
        {
            handler.OnError("invalid format");
            return begin;
        }

        const char* ptr = begin;
        while (ptr < end)
        {
            _TimeSpecPadding pad = _TimeSpecPadding::Unknown;

            if (*ptr == '}')
                break;

            if (*ptr != '%')
            {
                ++ptr;
                continue;
            }

            if (begin != ptr)
                handler.OnText(begin, ptr);

            ++ptr; // skip '%' character
            if (ptr == end)
            {
                handler.OnError("invalid format");
                return ptr;
            }

            switch (*ptr)
            {
                case '_': pad = _TimeSpecPadding::Space; ++ptr; break;
                case '-': pad = _TimeSpecPadding::None; ++ptr; break;
                case '0': pad = _TimeSpecPadding::Zero; ++ptr; break;
            }

            if (ptr == end)
            {
                handler.OnError("invalid format");
                return ptr;
            }

            const char ch = *ptr++;
            switch (ch)
            {
                // Special characters
                case '%': handler.OnText(ptr - 1, ptr); break;
                case 'n':
                {
                    const char newline[] = {'\n'};
                    handler.OnText(newline, newline + 1);
                    break;
                }
                case 't':
                {
                    const char tab[] = {'\t'};
                    handler.OnText(tab, tab + 1);
                    break;
                }

                // Year
                case 'Y': handler.OnYear(_TimeSpecAlt::None); break;
                case 'y': handler.OnShortYear(_TimeSpecAlt::None); break;
                case 'C': handler.OnCentury(_TimeSpecAlt::None); break;
                case 'G': handler.OnIsoWeekBasedYear(); break;
                case 'g': handler.OnIsoWeekBasedShortYear(); break;

                // Day of the week
                case 'a': handler.OnAbbrWeekday(); break;
                case 'A': handler.OnFullWeekday(); break;
                case 'w': handler.OnDec0Weekday(_TimeSpecAlt::None); break;
                case 'u': handler.OnDec1Weekday(_TimeSpecAlt::None); break;

                // Month
                case 'h': handler.OnAbbrMonth(); break;
                case 'b': handler.OnAbbrMonth(); break;
                case 'B': handler.OnFullMonth(); break;
                case 'm': handler.OnDecMonth(_TimeSpecAlt::None); break;

                // Day of the year/month
                case 'U': handler.OnDec0WeekOfYear(_TimeSpecAlt::None); break;
                case 'W': handler.OnDec1WeekOfYear(_TimeSpecAlt::None); break;
                case 'V': handler.OnIsoWeekOfYear(_TimeSpecAlt::None); break;
                case 'j': handler.OnDayOfYear(); break;
                case 'd': handler.OnDayOfMonth(_TimeSpecAlt::None); break;
                case 'e': handler.OnDayOfMonthSpace(_TimeSpecAlt::None); break;

                // Hour, minute, second
                case 'H': handler.On24Hour(_TimeSpecAlt::None, pad); break;
                case 'I': handler.On12Hour(_TimeSpecAlt::None, pad); break;
                case 'M': handler.OnMinute(_TimeSpecAlt::None, pad); break;
                case 'S': handler.OnSecond(_TimeSpecAlt::None, pad); break;

                // Other
                case 'c': handler.OnDatetime(_TimeSpecAlt::None); break;
                case 'x': handler.OnLocDate(_TimeSpecAlt::None); break;
                case 'X': handler.OnLocTime(_TimeSpecAlt::None); break;
                case 'D': handler.OnUsDate(); break;
                case 'F': handler.OnIsoDate(); break;
                case 'r': handler.On12HourTime(); break;
                case 'R': handler.On24HourTime(); break;
                case 'T': handler.OnIsoTime(); break;
                case 'p': handler.OnAmPm(); break;
                case 'z': handler.OnUtcOffset(_TimeSpecAlt::None); break;
                case 'Z': handler.OnTzName(); break;

                // Alternative representations
                case 'E':
                {
                    if (ptr == end)
                    {
                        handler.OnError("invalid format");
                        return ptr;
                    }
                    switch (*ptr++)
                    {
                        case 'Y': handler.OnYear(_TimeSpecAlt::AltRep); break;
                        case 'y': handler.OnOffsetYear(); break;
                        case 'C': handler.OnCentury(_TimeSpecAlt::AltRep); break;
                        case 'c': handler.OnDatetime(_TimeSpecAlt::AltRep); break;
                        case 'x': handler.OnLocDate(_TimeSpecAlt::AltRep); break;
                        case 'X': handler.OnLocTime(_TimeSpecAlt::AltRep); break;
                        case 'z': handler.OnUtcOffset(_TimeSpecAlt::AltRep); break;
                        default: handler.OnError("invalid format"); return ptr;
                    }
                    break;
                }

                // Alternative numeric systems
                case 'O':
                {
                    if (ptr == end)
                    {
                        handler.OnError("invalid format");
                        return ptr;
                    }
                    switch (*ptr++)
                    {
                        case 'y': handler.OnShortYear(_TimeSpecAlt::AltNum); break;
                        case 'm': handler.OnDecMonth(_TimeSpecAlt::AltNum); break;
                        case 'U': handler.OnDec0WeekOfYear(_TimeSpecAlt::AltNum); break;
                        case 'W': handler.OnDec1WeekOfYear(_TimeSpecAlt::AltNum); break;
                        case 'V': handler.OnIsoWeekOfYear(_TimeSpecAlt::AltNum); break;
                        case 'd': handler.OnDayOfMonth(_TimeSpecAlt::AltNum); break;
                        case 'e': handler.OnDayOfMonthSpace(_TimeSpecAlt::AltNum); break;
                        case 'w': handler.OnDec0Weekday(_TimeSpecAlt::AltNum); break;
                        case 'u': handler.OnDec1Weekday(_TimeSpecAlt::AltNum); break;
                        case 'H': handler.On24Hour(_TimeSpecAlt::AltNum, pad); break;
                        case 'I': handler.On12Hour(_TimeSpecAlt::AltNum, pad); break;
                        case 'M': handler.OnMinute(_TimeSpecAlt::AltNum, pad); break;
                        case 'S': handler.OnSecond(_TimeSpecAlt::AltNum, pad); break;
                        case 'z': handler.OnUtcOffset(_TimeSpecAlt::AltNum); break;
                        default: handler.OnError("invalid format"); return ptr;
                    }
                    break;
                }
                default:
                    handler.OnError("invalid format");
            }
            begin = ptr;
        }

        if (begin != ptr)
            handler.OnText(begin, ptr);

        return ptr;
    }

    // --------------------------------------------------------------------------------------------

    /// \internal
    class _TimeSpecChecker
    {
    public:
        constexpr _TimeSpecChecker(const FmtParseCtx& ctx) : m_ctx(ctx) {}

    public:
        void OnError(const char* msg) { m_ctx.OnError(msg); }
        constexpr void OnText(const char* begin, const char* end) { HE_UNUSED(begin, end); }
        constexpr void OnYear(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnShortYear(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnOffsetYear() {}
        constexpr void OnCentury(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnIsoWeekBasedYear() {}
        constexpr void OnIsoWeekBasedShortYear() {}
        constexpr void OnAbbrWeekday() {}
        constexpr void OnFullWeekday() {}
        constexpr void OnDec0Weekday(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnDec1Weekday(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnAbbrMonth() {}
        constexpr void OnFullMonth() {}
        constexpr void OnDecMonth(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnDec0WeekOfYear(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnDec1WeekOfYear(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnIsoWeekOfYear(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnDayOfYear() {}
        constexpr void OnDayOfMonth(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnDayOfMonthSpace(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void On24Hour(_TimeSpecAlt alt, _TimeSpecPadding pad) { HE_UNUSED(alt, pad); }
        constexpr void On12Hour(_TimeSpecAlt alt, _TimeSpecPadding pad) { HE_UNUSED(alt, pad); }
        constexpr void OnMinute(_TimeSpecAlt alt, _TimeSpecPadding pad) { HE_UNUSED(alt, pad); }
        constexpr void OnSecond(_TimeSpecAlt alt, _TimeSpecPadding pad) { HE_UNUSED(alt, pad); }
        constexpr void OnDatetime(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnLocDate(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnLocTime(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnUsDate() {}
        constexpr void OnIsoDate() {}
        constexpr void On12HourTime() {}
        constexpr void On24HourTime() {}
        constexpr void OnIsoTime() {}
        constexpr void OnAmPm() {}
        constexpr void OnUtcOffset(_TimeSpecAlt alt) { HE_UNUSED(alt); }
        constexpr void OnTzName() {}

    private:
        const FmtParseCtx& m_ctx;
    };

    /// \internal
    constexpr const char* _ParseTimeSpec(const FmtParseCtx& ctx, StringView& spec)
    {
        const char* it = ctx.Begin();
        if (it != ctx.End() && *it == ':')
            ++it;

        const char* end = it;
        while (end != ctx.End() && *end != '}')
            ++end;

        if (it < end)
        {
            _TimeSpecChecker checker{ ctx };
            end = _ParseTimeSpecFormat(it, end, checker);
            spec = { it, end };
        }

        return end;
    }

    // --------------------------------------------------------------------------------------------
    template <>
    struct Formatter<FmtLocalTime>
    {
        static constexpr StringView DefaultSpec = "%Y-%m-%dT%H:%M:%S%z";

        StringView spec{ DefaultSpec };

        constexpr const char* Parse(const FmtParseCtx& ctx) { return _ParseTimeSpec(ctx, spec); }

        void Format(String& out, const FmtLocalTime& t) const;
    };

    // --------------------------------------------------------------------------------------------
    template <>
    struct Formatter<SystemTime>
    {
        static constexpr StringView DefaultSpec = "%Y-%m-%dT%H:%M:%SZ";

        StringView spec{ DefaultSpec };

        constexpr const char* Parse(const FmtParseCtx& ctx) { return _ParseTimeSpec(ctx, spec); }

        void Format(String& out, const SystemTime& t) const;
    };

    // --------------------------------------------------------------------------------------------
    template <>
    struct Formatter<FmtUtcTime> : Formatter<SystemTime>
    {
        void Format(String& out, const FmtUtcTime& t) const
        {
            Formatter<SystemTime>::Format(out, t.time);
        }
    };

    // --------------------------------------------------------------------------------------------
    template <>
    struct Formatter<Duration>
    {
        enum class Period : uint8_t
        {
            Nanoseconds,    ///< 'n' = nanoseconds (ns)
            Microseconds,   ///< 'u' = microseconds (µs)
            Milliseconds,   ///< 'm' = milliseconds (ms)
            Seconds,        ///< 'S' = seconds (s)
            Minutes,        ///< 'M' = minutes (m)
            Hours,          ///< 'H' = hours (h)
            Days,           ///< 'd' = days (D)
            Weeks,          ///< 'w' = weeks (W)
            Months,         ///< 'o' = months (M)
            Years,          ///< 'y' = years (Y)
        };

        bool suffix = true;
        char type = 'i';
        int32_t precision = -1;
        Period period = Period::Nanoseconds;

        constexpr const char* Parse(const FmtParseCtx& ctx)
        {
            const char* it = ctx.Begin();
            if (it != ctx.End() && *it == ':')
                ++it;

            const char* end = it;
            while (end != ctx.End() && *end != '}')
                ++end;

            while (it < end)
            {
                it = ParseFmtSpecPrecision(it, end, ctx, precision);
                if (it == end)
                    break;

                char c = *it++;

                switch (c)
                {
                    case 'i':
                    case 'a':
                    case 'A':
                    case 'e':
                    case 'E':
                    case 'f':
                    case 'F':
                    case 'g':
                    case 'G':
                        type = c;
                        break;
                    case '#': suffix = false; break;
                    case 'n': period = Period::Nanoseconds; break;
                    case 'u': period = Period::Microseconds; break;
                    case 'm': period = Period::Milliseconds; break;
                    case 'S': period = Period::Seconds; break;
                    case 'M': period = Period::Minutes; break;
                    case 'H': period = Period::Hours; break;
                    case 'd': period = Period::Days; break;
                    case 'w': period = Period::Weeks; break;
                    case 'o': period = Period::Months; break;
                    case 'y': period = Period::Years; break;
                    default:
                        ctx.OnError("invalid format specifier");
                        return it;
                }
            }

            if (type == 'i' && precision != -1)
                ctx.OnError("invalid format specifier, integers cannot specify precision.");

            return it;
        }

        void Format(String& out, const Duration& d) const;
    };
}
