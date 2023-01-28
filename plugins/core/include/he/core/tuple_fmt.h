// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/tuple.h"

namespace he
{
    template <typename... Types>
    struct Formatter<Tuple<Types...>>
    {
        using Type = Tuple<Types...>;

        constexpr const char* Parse(const FmtParseCtx& ctx) const { return ctx.Begin(); }

        void Format(String& out, const Type& t) const
        {
            if constexpr (Type::Size == 0)
            {
                out.Append("()");
            }
            else
            {
                TupleApply(t, [&](const auto& first, const auto&... rest)
                {
                    FormatTo(out, "({}", first);
                    (FormatTo(out, ", {}", rest), ...);
                    out.Append(')');
                });
            }
        }
    };
}
