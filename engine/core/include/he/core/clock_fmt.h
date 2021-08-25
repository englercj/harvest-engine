// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"

#include "fmt/format.h"

namespace fmt
{
    template <typename T>
    struct formatter<he::Time<T>>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::Time<T>& t, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return format_to(ctx.out(), "{}ns", t.ns);
        }
    };

    template <>
    struct formatter<he::Duration>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::Duration& d, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return format_to(ctx.out(), "{}ns", d.ns);
        }
    };
}
