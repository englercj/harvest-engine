// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/vector.h"

namespace he
{
    template <typename T>
    struct Formatter<Vector<T>>
    {
        using Type = Vector<T>;

        constexpr const char* Parse(const FmtParseCtx& ctx) { return ctx.Begin(); }

        void Format(String& out, const Vector<T>& value) const
        {
            FormatTo(out, "({})", FmtJoin(value, ", "));
        }
    };
}
