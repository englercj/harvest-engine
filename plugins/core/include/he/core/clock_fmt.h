// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/utils.h"

#include "fmt/core.h"

#include <algorithm>
#include <ctime>

namespace he
{
    struct FmtLocalTime
    {
        explicit FmtLocalTime(SystemTime t) : time(t) {}
        SystemTime time;
    };

    struct FmtUtcTime
    {
        explicit FmtUtcTime(SystemTime t) : time(t) {}
        SystemTime time;
    };
}

namespace fmt
{
    struct _TimeFormatter
    {
        static constexpr he::StringView DefaultSpec = "%Y-%m-%d %H:%M:%S";

        he::StringView spec{ DefaultSpec };

        constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
        {
            auto it = ctx.begin();
            if (it != ctx.end() && *it == ':')
                ++it;

            auto end = it;
            while (end != ctx.end() && *end != '}')
                ++end;

            spec = { it, end };
            return end;
        }

        template <typename FormatContext>
        auto format(const std::tm& tm, FormatContext& ctx) const -> decltype(ctx.out())
        {
            // By appending an extra space we can distinguish an empty result that
            // indicates insufficient buffer size from a guaranteed non-empty result
            // https://github.com/fmtlib/fmt/issues/2238
            he::String format = spec;
            format.PushBack(' ');

            he::String buf;
            while (true)
            {
                const uint32_t size = buf.Capacity();
                const size_t count = std::strftime(buf.Data(), size, format.Data(), &tm);
                if (count != 0)
                {
                    buf.Resize(static_cast<uint32_t>(count));
                    break;
                }
                buf.Reserve(buf.Capacity() + he::Max<uint32_t>(size, 32));
            }

            // The `-1` is to remove the extra space that we added to the format earlier
            return std::copy(buf.begin(), buf.end() - 1, ctx.out());
        }
    };

    template <>
    struct formatter<he::FmtLocalTime> : _TimeFormatter
    {
        template <typename FormatContext>
        auto format(const he::FmtLocalTime& t, FormatContext& ctx) const -> decltype(ctx.out())
        {
            const std::time_t time = t.time.val / he::Seconds::Ratio;
            std::tm tm;
            localtime_s(&tm, &time);

            return _TimeFormatter::format(tm, ctx);
        }
    };

    template <>
    struct formatter<he::FmtUtcTime> : _TimeFormatter
    {
        template <typename FormatContext>
        auto format(const he::FmtUtcTime& t, FormatContext& ctx) const -> decltype(ctx.out())
        {
            const std::time_t time = t.time.val / he::Seconds::Ratio;
            std::tm tm;
            gmtime_s(&tm, &time);

            return _TimeFormatter::format(tm, ctx);
        }
    };

    template <>
    struct formatter<he::Duration>
    {
        he::StringView spec;

        constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
        {
            auto it = ctx.begin();
            if (it != ctx.end() && *it == ':')
                ++it;

            auto end = it;
            while (end != ctx.end() && *end != '}')
                ++end;

            spec = { it, end };
            return end;
        }

        template <typename FormatContext>
        auto format(const he::Duration& d, FormatContext& ctx) const -> decltype(ctx.out())
        {
            auto out = ctx.out();

            const char* s = spec.Begin();
            const char* end = spec.End();

            he::Duration duration = d;

            while (s < end)
            {
                if (*s != '%')
                {
                    *out++ = *s++;
                    continue;
                }

                ++s;
                switch (*s++)
                {
                    case '%': *out++ = '%'; break;
                    case 'n': *out++ = '\n'; break;
                    case 't': *out++ = '\t'; break;
                    case 'N': out = WritePeriod<he::Nanoseconds>(out, duration); break;
                    case 'u': out = WritePeriod<he::Microseconds>(out, duration); break;
                    case 'm': out = WritePeriod<he::Milliseconds>(out, duration); break;
                    case 's': out = WritePeriod<he::Seconds>(out, duration); break;
                    case 'M': out = WritePeriod<he::Minutes>(out, duration); break;
                    case 'H': out = WritePeriod<he::Hours>(out, duration); break;
                    case 'D': out = WritePeriod<he::Days>(out, duration); break;
                    case 'W': out = WritePeriod<he::Weeks>(out, duration); break;
                    default:
                        HE_ASSERT(false, "Invalid format character");
                        return out;
                }
            }

            return out;
        }

        template <typename T, typename U>
        auto WritePeriod(U out, he::Duration& duration) const
        {
            const int64_t count = he::ToPeriod<T>(duration);
            duration.val -= he::FromPeriod<T>(count).val;
            return format_to(out, "{}", count);
        }
    };
}
