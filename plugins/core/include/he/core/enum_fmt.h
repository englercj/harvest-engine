// Copyright Chad Engler

#pragma once

#include "he/core/type_traits.h"

#include "fmt/format.h"

namespace he
{
    template <he::Enum T> const char* AsString(T x);
}

namespace fmt
{
    template <he::Enum T>
    struct formatter<T>
    {
        constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(T s, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return format_to(ctx.out(), he::AsString(s));
        }
    };
}
