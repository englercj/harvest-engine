// Copyright Chad Engler

#pragma once

#include "he/core/enum_ops.h"
#include "he/core/type_traits.h"

#include "fmt/core.h"

#include <type_traits>

namespace fmt
{
    template <he::Enum T>
    struct formatter<T>
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
        auto format(T s, FormatContext& ctx) const -> decltype(ctx.out())
        {
            switch (spec)
            {
                case 'v':
                    return fmt::format_to(ctx.out(), "{}", static_cast<std::underlying_type_t<T>>(s));
                case 's':
                    return fmt::format_to(ctx.out(), "{}", he::AsString(s));
                default:
                    return fmt::format_to(ctx.out(), "({}){}", static_cast<std::underlying_type_t<T>>(s), he::AsString(s));
            }
        }
    };
}
