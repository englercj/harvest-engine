// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/hash.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    template <AnyOf<MD5::ValueType, SHA1::ValueType, SHA256::ValueType> T>
    struct Formatter<T>
    {
        using Type = T;

        bool lower = true;

        constexpr const char* Parse(const FmtParseCtx& ctx)
        {
            const char* it = ctx.Begin();
            if (it != ctx.End() && *it == ':')
                ++it;

            const char* end = it;
            while (end != ctx.End() && *end != '}')
                ++end;

            while (it < end)
            {
                char c = *it++;

                switch (c)
                {
                    case 'x': lower = true; break;
                    case 'X': lower = false; break;
                    default:
                        ctx.OnError("invalid format specifier");
                        return it;
                }
            }

            return it;
        }

        void Format(String& out, const T& value) const
        {
            if (lower)
                return FormatTo(out, "{:02x}", FmtJoin(value.bytes, ""));

            return FormatTo(out, "{:02X}", FmtJoin(value.bytes, ""));
        }
    };
}
