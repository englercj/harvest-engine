// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/span.h"

namespace he
{
    template <typename T>
    struct Formatter<Span<T>>
    {
        using Type = Span<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) { return ctx.Begin(); }

        void Format(String& out, const Span<T>& value) const
        {
            FormatTo(out, "({})", FmtJoin(value, ", "));
        }
    };
}
