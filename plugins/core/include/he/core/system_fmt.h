// Copyright Chad Engler

#pragma once

#include "he/core/system.h"

#include "fmt/core.h"

namespace fmt
{

    template <>
    struct formatter<he::SystemInfo>
    {
        constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::SystemInfo& info, FormatContext& ctx) -> decltype(ctx.out())
        {
            return fmt::format_to(ctx.out(), "{} ({}.{}.{}.{})",
                info.platform, info.version.major, info.version.minor, info.version.patch, info.version.build);
        }
    };
}
