// Copyright Chad Engler

#pragma once

#include "he/core/type_info.h"
#include "he/core/string_view_fmt.h"

#include "fmt/core.h"

namespace fmt
{
    template <>
    struct formatter<he::TypeInfo>
    {
        char spec = '\0';

        constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
        {
            auto it = ctx.begin();
            if (it != ctx.end() && *it == ':')
                ++it;

            auto end = it;
            while (end != ctx.end() && *end != '}')
                ++end;

            if (it < end)
                spec = *it;

            return end;
        }

        template <typename FormatContext>
        auto format(const he::TypeInfo& info, FormatContext& ctx) const -> decltype(ctx.out())
        {
            switch (spec)
            {
                case 'v':
                    return fmt::format_to(ctx.out(), "{}", info.Hash());
                case 's':
                    return fmt::format_to(ctx.out(), "{}", info.Name());
                default:
                    return fmt::format_to(ctx.out(), "({}){}", info.Hash(), info.Name());
            }
        }
    };
}
