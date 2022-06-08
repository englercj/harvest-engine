#pragma once

#include "he/core/string.h"

#include "fmt/core.h"

namespace fmt
{
    template <>
    struct formatter<he::String>
    {
        constexpr auto parse(fmt::format_parse_context& ctx) const -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::String& s, FormatContext& ctx) const -> decltype(ctx.out())
        {
            return fmt::format_to(ctx.out(), "{}", fmt::string_view(s.Data(), s.Size()));
        }
    };
}
