// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/assets/types.h"

namespace he
{
    template <typename T>
    struct Formatter<assets::_UuidWrapper<T>>
    {
        using Type = assets::_UuidWrapper<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const assets::_UuidWrapper<T>& id) const
        {
            constexpr uint32_t ByteSize = sizeof(id.val.m_bytes);
            const uint8_t* b = id.val.m_bytes;
            return FormatTo(out, "{:02x}", FmtJoin(b, b + ByteSize, ""));
        }
    };

    template <typename T>
    struct Formatter<assets::_HashId<T>>
    {
        using Type = assets::_HashId<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const assets::_HashId<T>& id) const
        {
            return FormatTo(out, "{:#010x}", id.val);
        }
    };
}
