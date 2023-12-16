// Copyright Chad Engler

#pragma once

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/compiler.h"
#include "he/core/fmt.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/utils.h"

#include <time.h>

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

    struct _TimeFormatter
    {
    protected:
        constexpr const char* ParseInternal(const FmtParseCtx& ctx, StringView& spec)
        {
            const char* it = ctx.Begin();
            if (it != ctx.End() && *it == ':')
                ++it;

            const char* end = it;
            while (end != ctx.End() && *end != '}')
                ++end;

            if (it < end)
                spec = { it, end };

            return end;
        }

        void FormatInternal(String& out, StringView spec, const struct tm& tm) const
        {
            // TODO: Fractional seconds, strftime doesn't support it.

            // By appending an extra space we can distinguish an empty result that
            // indicates insufficient buffer size from a guaranteed non-empty result
            // https://github.com/fmtlib/fmt/issues/2238
            String format = spec;
            format.PushBack(' ');

            const uint32_t offset = out.Size();
            uint32_t size = String::MaxEmbedCharacters;
            out.Resize(offset + size, DefaultInit);
            while (true)
            {
                const size_t count = strftime(out.Data() + offset, size, format.Data(), &tm);
                if (count != 0)
                {
                    out.Resize(offset + static_cast<uint32_t>(count - 1));
                    break;
                }
                size *= 2;
                out.Resize(offset + size);
            }
        }
    };

    template <>
    struct Formatter<FmtLocalTime> : _TimeFormatter
    {
        static constexpr StringView DefaultSpec = "%Y-%m-%dT%H:%M:%S%z";

        StringView spec{ DefaultSpec };

        constexpr const char* Parse(const FmtParseCtx& ctx)
        {
            return ParseInternal(ctx, spec);
        }

        void Format(String& out, const FmtLocalTime& t) const
        {
            const time_t time = t.time.val / Seconds::Ratio;
            struct tm tm{};
        #if HE_COMPILER_MSVC
            localtime_s(&tm, &time);
        #else
            localtime_r(&time, &tm);
        #endif

            return FormatInternal(out, spec, tm);
        }
    };

    template <>
    struct Formatter<SystemTime> : _TimeFormatter
    {
        static constexpr StringView DefaultSpec = "%Y-%m-%dT%H:%M:%SZ";

        StringView spec{ DefaultSpec };

        constexpr const char* Parse(const FmtParseCtx& ctx)
        {
            return ParseInternal(ctx, spec);
        }

        void Format(String& out, const SystemTime& t) const
        {
            const time_t time = t.val / Seconds::Ratio;
            struct tm tm{};
        #if HE_COMPILER_MSVC
            gmtime_s(&tm, &time);
        #else
            gmtime_r(&time, &tm);
        #endif

            return FormatInternal(out, spec, tm);
        }
    };

    template <>
    struct Formatter<FmtUtcTime> : Formatter<SystemTime>
    {
        void Format(String& out, const FmtUtcTime& t) const
        {
            Formatter<SystemTime>::Format(out, t.time);
        }
    };

    template <>
    struct Formatter<Duration>
    {
        static constexpr StringView DefaultSpec = "%nns";

        StringView spec{ DefaultSpec };

        constexpr const char* Parse(const FmtParseCtx& ctx)
        {
            const char* it = ctx.Begin();
            if (it != ctx.End() && *it == ':')
                ++it;

            const char* end = it;
            while (end != ctx.End() && *end != '}')
                ++end;

            if (it < end)
                spec = { it, end };

            return end;
        }

        void Format(String& out, const Duration& d) const
        {
            const char* s = spec.Begin();
            const char* end = spec.End();

            Duration duration = d;

            while (s < end)
            {
                if (*s != '%')
                {
                    out.Append(*s++);
                    continue;
                }

                ++s;
                switch (*s++)
                {
                    case '%': out.Append('%'); break;
                    case 'n': WritePeriod<Nanoseconds>(out, duration); break;
                    case 'u': WritePeriod<Microseconds>(out, duration); break;
                    case 'm': WritePeriod<Milliseconds>(out, duration); break;
                    case 'S': WritePeriod<Seconds>(out, duration); break;
                    case 'M': WritePeriod<Minutes>(out, duration); break;
                    case 'H': WritePeriod<Hours>(out, duration); break;
                    case 'D': WritePeriod<Days>(out, duration); break;
                    case 'W': WritePeriod<Weeks>(out, duration); break;
                    default:
                        HE_ASSERT(false, HE_MSG("Invalid format character"), HE_KV(char, s[-1]));
                }
            }
        }

        template <typename T>
        static void WritePeriod(String& out, Duration& duration)
        {
            const int64_t count = ToPeriod<T>(duration);
            duration.val -= FromPeriod<T>(count).val;
            FormatTo(out, "{}", count);
        }
    };
}
