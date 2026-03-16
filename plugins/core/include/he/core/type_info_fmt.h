// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/type_info.h"

namespace he
{
    template <>
    struct Formatter<TypeInfo>
    {
        using Type = TypeInfo;

        char spec{};

        constexpr const char* Parse(const FmtParseCtx& ctx)
        {
            spec = 0;

            const char* it = ctx.Begin();
            if (it != ctx.End() && *it == ':')
                ++it;

            const char* end = it;
            while (end != ctx.End() && *end != '}')
                ++end;

            if (it < end)
                spec = *it;

            if (spec != 0 && spec != 's' && spec != 'd')
                ctx.OnError("Unknown type specifier");

            return end;
        }

        void Format(String& out, const TypeInfo& info) const
        {
            switch (spec)
            {
                case 's':
                    return FormatTo(out, "{}", info.Name());
                case 'd':
                    return FormatTo(out, "{}", info.Hash());
                default:
                    return FormatTo(out, "({}){}", info.Hash(), info.Name());
            }
        }
    };
}
