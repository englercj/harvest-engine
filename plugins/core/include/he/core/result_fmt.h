// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"

#include "fmt/core.h"

namespace fmt
{
    template <>
    struct formatter<he::Result>
    {
        constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::Result& result, FormatContext& ctx) -> decltype(ctx.out())
        {
            const he::String msg = result.ToString(he::CrtAllocator::Get());
            return format_to(ctx.out(), "({}) {}", result.GetCode(), msg);
        }
    };
}
