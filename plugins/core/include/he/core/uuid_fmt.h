// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/uuid.h"

namespace he
{
    template <>
    struct Formatter<Uuid>
    {
        using Type = Uuid;

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

        void Format(String& out, const Uuid& uuid) const
        {
            // 00000000-0000-0000-0000-000000000000
            constexpr char LowerFmt[] = "{:02x}-{:02x}-{:02x}-{:02x}-{:02x}";
            constexpr char UpperFmt[] = "{:02X}-{:02X}-{:02X}-{:02X}-{:02X}";

            static_assert(sizeof(uuid.m_bytes) == 16);
            const uint8_t* b = uuid.m_bytes;

            const char* fmtStr = lower ? LowerFmt : UpperFmt;
            return FormatTo(out, FmtRuntime(fmtStr),
                FmtJoin(b + 0, b + 4, ""),
                FmtJoin(b + 4, b + 6, ""),
                FmtJoin(b + 6, b + 8, ""),
                FmtJoin(b + 8, b + 10, ""),
                FmtJoin(b + 10, b + 16, ""));
        }
    };
}
