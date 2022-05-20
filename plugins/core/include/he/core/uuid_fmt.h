// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/uuid.h"

#include "fmt/core.h"

namespace fmt
{
    template <>
    struct formatter<he::Uuid>
    {
        constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::Uuid& uuid, FormatContext& ctx) -> decltype(ctx.out())
        {
            const he::String msg = uuid.ToString(he::Allocator::GetTemp());
            return fmt::format_to(ctx.out(), "{}", msg);
        }
    };
}
