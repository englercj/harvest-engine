// Copyright Chad Engler

#pragma once

#include "he/core/ascii.h"
#include "he/core/span.h"

#include "fmt/core.h"

namespace fmt
{
    template <>
    struct formatter<he::Span<const uint8_t>>
    {
        constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(he::Span<const uint8_t> s, FormatContext& ctx) -> decltype(ctx.out())
        {
            auto out = ctx.out();
            for (uint32_t i = 0; i < s.Size(); ++i)
            {
                *out++ = he::ToHex((s[i] & 0xF0) >> 4);
                *out++ = he::ToHex(s[i] & 0x0F);
            }
            return out;
        }
    };
}
