// Copyright Chad Engler

#pragma once

#include "he/core/type_traits.h"

#include "fmt/format.h"

namespace fmt
{
    template <typename T>
    struct formatter<T, std::enable_if_t<he::IsEnum<T>>>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(T s, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return format_to(ctx.out(), AsString(s));
        }
    };
}
