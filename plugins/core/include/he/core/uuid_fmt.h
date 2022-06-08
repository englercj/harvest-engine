// Copyright Chad Engler

#pragma once

#include "he/core/uuid.h"

#include "fmt/format.h"
#include "fmt/ranges.h"

namespace fmt
{
    template <>
    struct formatter<he::Uuid>
    {
        bool lower = true;

        constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
        {
            auto it = ctx.begin();
            if (it != ctx.end() && *it == ':')
                ++it;

            auto end = it;
            while (end != ctx.end() && *end != '}')
                ++end;

            while (it < end)
            {
                char c = *it++;

                switch (c)
                {
                    case 'x': lower = true; break;
                    case 'X': lower = false; break;
                    default:
                        ctx.on_error("invalid format specifier");
                        return it;
                }
            }

            return it;
        }

        template <typename FormatContext>
        auto format(const he::Uuid& uuid, FormatContext& ctx) -> decltype(ctx.out())
        {
            // 00000000-0000-0000-0000-000000000000
            constexpr char LowerFmt[] = "{:x}-{:x}-{:x}-{:x}-{:x}";
            constexpr char UpperFmt[] = "{:X}-{:X}-{:X}-{:X}-{:X}";

            static_assert(sizeof(uuid.m_bytes) == 16);
            const uint8_t* b = uuid.m_bytes;

            const char* fmtStr = lower ? LowerFmt : UpperFmt;
            return fmt::format_to(ctx.out(), fmt::runtime(fmtStr),
                fmt::join(b + 0, b + 4, ""),
                fmt::join(b + 4, b + 6, ""),
                fmt::join(b + 6, b + 8, ""),
                fmt::join(b + 8, b + 10, ""),
                fmt::join(b + 10, b + 16, ""));
        }
    };
}
