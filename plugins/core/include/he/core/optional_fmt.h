// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/optional.h"

namespace he
{
    template <typename T>
    struct Formatter<Optional<T>>
    {
        using Type = Optional<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) { return ctx.Begin(); }

        void Format(String& out, const Type& value) const
        {
            if (value.HasValue())
            {
                FormatTo(out, "{}", value.Value());
            }
            else
            {
                FormatTo(out, "<empty>");
            }
        }
    };
}
