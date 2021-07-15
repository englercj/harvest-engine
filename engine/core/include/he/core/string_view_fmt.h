#pragma once

#include "he/core/string_view.h"

#include "fmt/format.h"

namespace fmt
{
    template <>
    struct formatter<he::StringView>
    {
        constexpr auto parse(fmt::format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(he::StringView s, FormatContext& ctx) const -> decltype(ctx.out())
        {
            auto out = ctx.out();
            for (const char c : s)
                *(out++) = c;
            return out;
        }
    };
}
