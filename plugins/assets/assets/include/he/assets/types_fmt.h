// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"

#include "fmt/format.h"
#include "fmt/ranges.h"

#include <type_traits>

namespace fmt
{
    template <typename T>
    struct formatter<he::assets::_UuidWrapper<T>>
    {
        constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::assets::_UuidWrapper<T>& id, FormatContext& ctx) -> decltype(ctx.out())
        {
            constexpr uint32_t ByteSize = sizeof(id.val.m_bytes);
            const uint8_t* b = id.val.m_bytes;
            return fmt::format_to(ctx.out(), "{:02x}", fmt::join(b, b + ByteSize, ""));
        }
    };

    template <typename T>
    struct formatter<he::assets::_HashId<T>>
    {
        constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
        {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const he::assets::_HashId<T>& id, FormatContext& ctx) -> decltype(ctx.out())
        {
            return fmt::format_to(ctx.out(), "{:#010x}", id.val);
        }
    };
}
